#include <gmsh.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr double tankWidth = 0.100;
constexpr double tankHeight = 0.150;
constexpr double thickness = 0.001;

constexpr std::array<double, 6> xCoord = {0.000, 0.040, 0.049, 0.051, 0.060, 0.100};
constexpr std::array<int, 5> cellsX = {40, 36, 8, 36, 40};

constexpr std::array<double, 3> yCoord = {0.000, 0.100, 0.150};
constexpr std::array<int, 2> cellsY = {400, 100};

constexpr int nxBlocks = static_cast<int>(cellsX.size());
constexpr int nyBlocks = static_cast<int>(cellsY.size());
constexpr double tol = 1e-8;

bool near(double a, double b) { return std::abs(a - b) < tol; }

int transfiniteLine(int startPoint, int endPoint, int cells) {
    const int curve = gmsh::model::geo::addLine(startPoint, endPoint);
    gmsh::model::geo::mesh::setTransfiniteCurve(curve, cells + 1);
    return curve;
}

void physicalGroup(int dimension, const std::set<int>& entities, int groupTag, const std::string& name) {
    if (entities.empty()) { throw std::runtime_error("Physical group is empty: " + name); }
    const std::vector<int> tags(entities.begin(), entities.end());
    gmsh::model::addPhysicalGroup(dimension, tags, groupTag);
    gmsh::model::setPhysicalName(dimension, groupTag, name);
}
}

int main() {
    bool initialized = false;

    try {
        gmsh::initialize();
        initialized = true;
        gmsh::model::add("bubble_tank");

        std::array<std::array<int, nxBlocks + 1>, nyBlocks + 1> points{};
        for (int j = 0; j <= nyBlocks; ++j) {
            for (int i = 0; i <= nxBlocks; ++i) {
                points[j][i] = gmsh::model::geo::addPoint(
                    xCoord[i], yCoord[j], 0.0);
            }
        }

        std::array<std::array<int, nxBlocks>, nyBlocks + 1> horizontal{};
        for (int j = 0; j <= nyBlocks; ++j) {
            for (int i = 0; i < nxBlocks; ++i) {
                horizontal[j][i] = transfiniteLine(
                    points[j][i], points[j][i + 1], cellsX[i]);
            }
        }

        std::array<std::array<int, nxBlocks + 1>, nyBlocks> vertical{};
        for (int j = 0; j < nyBlocks; ++j) {
            for (int i = 0; i <= nxBlocks; ++i) {
                vertical[j][i] = transfiniteLine(
                    points[j][i], points[j + 1][i], cellsY[j]);
            }
        }

        std::vector<int> baseSurfaces;
        baseSurfaces.reserve(nxBlocks * nyBlocks);

        for (int j = 0; j < nyBlocks; ++j) {
            for (int i = 0; i < nxBlocks; ++i) {
                const int loop = gmsh::model::geo::addCurveLoop({ horizontal[j][i], vertical[j][i + 1],
                    -horizontal[j + 1][i], -vertical[j][i]});

                const int surface = gmsh::model::geo::addPlaneSurface({loop});
                gmsh::model::geo::mesh::setTransfiniteSurface(surface);
                gmsh::model::geo::mesh::setRecombine(2, surface);
                baseSurfaces.push_back(surface);
            }
        }

        std::set<int> volumes;
        for (const int surface : baseSurfaces) {
            gmsh::vectorpair extruded;
            gmsh::model::geo::extrude({{2, surface}}, 0.0, 0.0, thickness,
                extruded, {1}, {1.0}, true);

            for (const auto& entity : extruded) {
                if (entity.first == 3) {
                    volumes.insert(entity.second);
                }
            }
        }
        if (volumes.size() != baseSurfaces.size()) {
            throw std::runtime_error("Unexpected number of extruded volumes");
        }

        gmsh::model::geo::synchronize();

        std::set<int> walls, gasInlet, atmosphere, frontAndBack;
        gmsh::vectorpair surfaces;
        gmsh::model::getEntities(surfaces, 2);

        for (const auto& entity : surfaces) {
            double xmin, ymin, zmin, xmax, ymax, zmax;
            gmsh::model::getBoundingBox(2, entity.second, xmin, ymin, zmin, xmax, ymax, zmax);

            if (near(zmin, zmax)) {
                // Front and back faces (z = 0 and z = thickness).
                frontAndBack.insert(entity.second);
            } else if ((near(xmin, 0.0) && near(xmax, 0.0)) ||
                       (near(xmin, tankWidth) && near(xmax, tankWidth))) {
                walls.insert(entity.second);
            } else if (near(ymin, 0.0) && near(ymax, 0.0)) {
                if (xmin >= xCoord[2] - tol && xmax <= xCoord[3] + tol) {
                    gasInlet.insert(entity.second);
                } else {
                    walls.insert(entity.second);
                }
            } else if (near(ymin, tankHeight) && near(ymax, tankHeight)) {
                atmosphere.insert(entity.second);
            }
        }

        physicalGroup(3, volumes,      1, "fluid");
        physicalGroup(2, walls,        2, "walls");
        physicalGroup(2, gasInlet,     3, "gasInlet");
        physicalGroup(2, atmosphere,   4, "atmosphere");
        physicalGroup(2, frontAndBack, 5, "frontAndBack");

        gmsh::model::mesh::generate(3);
        gmsh::option::setNumber("Mesh.MshFileVersion", 2.2);
        gmsh::option::setNumber("Mesh.Binary", 0);

        std::filesystem::create_directories("mesh");
        gmsh::write("mesh/tank.msh");

        const int nx = std::accumulate(cellsX.begin(), cellsX.end(), 0);
        const int ny = std::accumulate(cellsY.begin(), cellsY.end(), 0);
        std::cout << "Expected hexahedra: " << nx * ny << '\n'
                  << "X cells: " << nx << ", Y cells: " << ny << '\n'
                  << "Gas inlet width: " << 1000.0 * (xCoord[3] - xCoord[2])
                  << " mm\n"
                  << "Saved mesh/tank.msh\n";

        gmsh::finalize();
        initialized = false;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Mesh generation failed: " << error.what() << '\n';
        if (initialized) {
            gmsh::finalize();
        }
        return 1;
    }
}
