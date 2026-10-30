# Code and Port Architecture

The project separates portable application logic from platform-specific firmware.

* `Code/` contains the hardware-agnostic C11 kernel: measurement, providers, validation, calculation, and orchestration.  
* `Port/` contains platform-specific implementations and firmware integration, including peripheral drivers and target build configuration.

Dependencies are strictly one-way: `Port/` may depend on `Code/`, but `Code/` must never depend on `Port/` or target-specific hardware.

This separation keeps the application core portable and allows it to be reused across different hardware platforms.

* * *

## Related documents

* [`Software architecture diagram`](../Docs/software-architecture-diagram.md).  
* [`Data flow specification`](../Docs/data-flow-specification.md).  
* [`Contracts and conventions`](../Docs/contracts-and-conventions.md).  
* [`Doxygen source documentation`](https://cloclacordis.github.io/FieldEdge-Evapotranspiration/files.html).  
* [`Software architecture diagram`](../Docs/software-architecture-diagram.md).  
* [`README`](../README.md).
