# VOF inlet regression tests

Build with `BUILD_TESTING=ON`, then run:

```sh
cmake --build BUILD --target vof_boundary_test
ctest --test-dir BUILD -R '^vof-boundary' --output-on-failure
```

The tests exercise the actual liquid-advection and boundary routines with a
partial inlet surrounded by walls. They cover PLIC, THINC, THINC/WLIC and
THINC/AWLIC, all six domain faces, all six directional sweep orders, and two
components supplied in a 1:3 ratio. Four inlet cells with unit area and unit
normal speed must supply liquid volumes 0.1 and 0.3 over a time step of 0.1.
The PLIC test also checks that its three substeps restore the original time
step. Ghost refresh must leave interior values and other components intact.

Boundary tests check all allocated lower/upper ghost layers, solid-volume
exclusion, the aggregate liquid field used with solute diffusion, per-cell
inlet overrides, outward fluxes, OUT reverse-flow suppression, and untouched
MPI faces. Running under AddressSanitizer checks the lower-ghost allocation
limit that used to be exceeded by `bcfl_calc`.

With MPI enabled, `vof-boundary-mpi` additionally exchanges actual halos and
advects a liquid slab across a rank interface, checking that the receiving
rank gains liquid and the global volume remains four cells. The launcher can
be configured through the standard `MPIEXEC_PREFLAGS` CMake option.

The tests isolate boundary transport using prescribed divergence-free
velocities. They do not validate the coupled thermal/phase-change model,
CUDA execution, or the multilayer model's existing inlet-selection rule.
