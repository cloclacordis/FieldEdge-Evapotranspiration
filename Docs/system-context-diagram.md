## System context

The system operates at the edge, acquiring agrometeorological data from sensors, processing and validating the measurements, and calculating reference evapotranspiration (ETo). The resulting ETo value is provided to an external irrigation system for local irrigation decision-making and control. Version v0.1.x implements the measurement and computation pipeline on a PC using emulated sensor data. Version v0.2.x is the STM32 implementation under development with real sensors, LoRa communication, and real-time operation.

* * *

### System context diagram

```mermaid
flowchart LR
    ENV["Environmental<br/>conditions"]
    SENS["Sensors"]

    subgraph FieldEdge["FieldEdge"]
        direction LR
        MEAS["Measurement<br/>subsystem"]
        CALC["Computation<br/>kernel"]
        MEAS -->|"Measurement data"| CALC
    end

    DEC["Irrigation<br/>system"]

    ENV -->|"Physical quantities"| SENS
    SENS -->|"Sensor readings"| MEAS
    CALC -->|"ETo value"| DEC

    classDef external fill:#f7f7f7,stroke:#555,stroke-width:1.5px,color:#222
    classDef system fill:#eaf2f8,stroke:#2c5f85,stroke-width:2px,color:#111
    classDef internal fill:#fff,stroke:#2c5f85,stroke-width:1.5px,color:#111

    class ENV,SENS,DEC external
    class MEAS,CALC internal
```
