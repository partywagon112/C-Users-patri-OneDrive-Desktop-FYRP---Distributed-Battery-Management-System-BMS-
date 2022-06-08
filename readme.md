# Final Year Research Project - Distributed Battery Management System
## Design, Optimisation and System Integration for an Electric Solar Vehicle
## Patrick Curtain & Ethan Suter, Swinburne University of Technology

A project to design a battery management system as a testbed for how one would integrate an active balancing system for an electric vehicle, in particular, an electric solar vehicle.

In this project, a hypothetical system has been conceived for the Swinburne Solar Team electric vehicle and a BMS layed out for how it would function as part of the vehicle. The BMS in question is a multichemistry (0.8-5V), distributed system, comprised of a single gateway (Master) Central Management Unit (CMU) and multiple twelve cell circuit (Slaves) called Local Measurement Units (LMU), connected to the gateway on a private CANBUS. The project makes use of the LTC6811-2 and LTC3300-2, using conventional 4-Wire SPI to make the connection to a control interface more modular.

## BMS-LMU
![BMS-LMU](./Images/three_quater.jpg)

As a proof of concept, a test LMU was produced to perform the specified functions of the BMS-LMU, and documentation included in this repo.

## Acknowledgements
A big thank you to Team Swinburne for the platform and code base used throughout the design of this project. Their work is available: https://github.com/Team-Swinburne