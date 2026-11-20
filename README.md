# Bubble Simulation

2D simulation of air bubble formation and rising in water using Gmsh and OpenFOAM 14.

## Requirements

- macOS (Apple Silicon)
- CMake and Gmsh
- Docker Desktop
- ParaView (optional, for visualization)

Install CMake and Gmsh:

```bash
brew install cmake gmsh
```

## 1. Generate the mesh

Run from the project root:

```bash
cmake -S . -B build
cmake --build build
./build/mesh_generator
```

The generated mesh contains 80,000 hexahedral cells and is saved to `mesh/tank.msh`.

## 2. Build the Docker image

Make sure Docker Desktop is running.

```bash
docker build --platform linux/arm64 \
  -t bubble-openfoam:14 \
  -f docker/Dockerfile .
```

## 3. Run the simulation

Start the OpenFOAM container from the project root:

```bash
docker run --rm -it --platform linux/arm64 \
  -v "$(pwd):/workspace" \
  -w /workspace \
  bubble-openfoam:14 bash
```

Inside the container, load OpenFOAM:

```bash
source /opt/openfoam14/etc/bashrc
```

Import the mesh and configure the 2D boundaries (first run only):

```bash
bash scripts/prepare_case.sh
```

Start the simulation:

```bash
foamRun -case case | tee log.foamRun
```

The simulation runs from `t = 0` to `t = 2 s`. Results are saved in the `case/` directory.

## 4. Visualize the results

Exit the container:

```bash
exit
```

Create a ParaView marker file on the host:

```bash
touch case/case.foam
```

Open `case/case.foam` in ParaView, click **Apply**, select the `alpha.water` field, and use the time controls to view the simulation.

`alpha.water = 1` represents water, while `alpha.water = 0` represents air.