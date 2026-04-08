**JUPITER numerically reproduces, using computational fluid dynamics, material melting behavior associated with temperature rise as well as molten material migration and solidification behavior. All phases are treated as fluids. The main features are summarized below.**

***

## Numerical Model Features

*   Targets incompressible viscous flows with phase interfaces; the finite difference method is employed.
*   Capable of treating three phases: solid, liquid, and gas.
*   Phase change (liquid ⇔ solid) can be simulated; a one‑fluid model is used for multiphase flow.
*   Interface‑capturing methods that can handle large interface deformations are employed (PLIC, THINC, etc.).
*   The behavior of an arbitrary number of fluid components can be analyzed.
*   Liquid/solid/gas phases and material components are identified using volume fraction (Volume of Fluid: VOF) functions.
*   Radiative heat transfer can be calculated using the Discrete Ordinate Method.
*   Remelting processes can be simulated by pseudo‑input of internal heat sources such as decay heat.
*   Chemical reactions (eutectic reactions, steam oxidation reactions) can be modeled.
*   Flow in porous media can be analyzed using the Darcy–Brinkman equation.
*   The solid phase is represented using an Immersed Boundary Method.
*   Surface tension is evaluated using the CSF (Continuum Surface Force) model, and phase change is evaluated using the temperature recovery method.
*   The pressure Poisson equation is solved using a preconditioned MPI–OpenMP hybrid CG solver developed by the Center for Computational Science.
*   For spatial discretization:
    *   Advection equations: selectable among 5th‑order WENO, 3rd‑order upwind, and 1st‑order upwind schemes.
    *   Interface advection equations: selectable among THINC, THINC/WLIC, THINC/AWLIC, and PLIC methods.
    *   Diffusion equations: 2nd‑order central difference scheme is used.

***

## Computational Code Features

*   Written in C and CUDA; uses orthogonal uniform grids.
*   Full three‑directional (x, y, z) parallelization is implemented using MPI.

