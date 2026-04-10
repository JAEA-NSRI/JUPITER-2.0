# JUPITER

## References to be cited

   S. Yamashita, T. Ina, Y. Idomura and H. Yoshida, “A numerical simulation method for molten material behavior in nuclear reactors,” Nucl. Eng. Des., 332, pp. 301-312 (2017). https://www.sciencedirect.com/science/article/pii/S0029549317303035


## Manual

   S. Yamashita, "Development of Multiphase and Multicomponent Detailed Thermal Hydraulics Code JUPITER", JAEA-Data/Code 2025-[002(Japanese)](https://jopss.jaea.go.jp/pdfdata/JAEA-Data-Code-2025-002.pdf)/[003(English)](https://jopss.jaea.go.jp/pdfdata/JAEA-Data-Code-2025-002.pdf)

## Code Description

JUPITER is an analysis code developed with the objective of achieving an advanced understanding of the complex melting and relocation phenomena occurring inside a reactor core during severe accidents (SA). It is capable of accurately simulating multiphase and multicomponent flows consisting of gas, liquid, and solid phases with arbitrary compositions.

The application scope of JUPITER covers a wide range of phenomena, including core melting behavior in the early stages of severe accidents—taking into account eutectic reactions and steam oxidation reactions—as well as gas–liquid two‑phase flow analysis within fuel assemblies. In addition, JUPITER can be applied to the analysis of air‑cooling behavior of debris in the containment vessel using porous media models.
Furthermore, JUPITER is equipped with a multicomponent analysis capability that enables unified treatment of multicomponent systems regardless of whether the materials are in solid, liquid, or gaseous states. It also provides modeling functions for solid–liquid phase change and the ability to directly import and analyze complex structural geometries from 3D‑CAD data. In addition, chemical reactions such as eutectic reactions, steam oxidation reactions, and hydrog##en absorption reactions can be evaluated in conjunction with thermodynamic databases. 

By leveraging large‑scale parallel computation based on a hybrid MPI–OpenMP framework and GPGPU implementation using CUDA, JUPITER achieves high computational performance and supports simulations with mesh sizes on the order of hundreds of billions of grid cells. Moreover, JUPITER enables porous media flow analysis based on the Darcy–Brinkman equation and radiative heat transfer analysis using the discrete ordinate (DO) method. These features allow comprehensive reproduction of the diverse and strongly coupled physical phenomena that characterize severe accidents, which constitutes one of the major strengths of the code. The main features are summarized below.**

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

## Computational Code Features

*   Written in C, Fortran and CUDA; uses orthogonal uniform grids.
*   Full three‑directional (x, y, z) parallelization is implemented using MPI.
