.. _data-handling:
Data Handling
==============

.. uml::
    title Time Handling for a Single Message
    hide footbox

    actor User
    database "RabbitMQ Server" as server
    boundary "Execution of Co-Simulation" as exec
    participant RabbitMQFMU as FMU

    User -> exec: Start

    == Initialization ==
    exec -> FMU: EnterInitializationMode()"
    FMU -> server: createTopicExchangeBindingAndQueue
    ... RabbitMQ FMU Blocks execution until data is available in the topic ...
    server -> FMU: "Publish message with timestamp of WCT1"
    note over FMU: Rabbit FMU unblocks and sets Simulation Time 0 = WCT1.

    == Simulation Loop until endtime ==
    exec -> FMU: Fmi2DoStep(currentSimulationTime, simulaionStepSize)
        loop FMU process messages until messageTimeStampInSimulationTime >= currentSimulationTime + simulationStepSize
        server -> FMU: Message
        alt messageTimeStampInSimulationTime <= currentSimulationTime+simulationStepSize
            FMU -> FMU: Update output values with values from message
        else messageTimeStampInSimulationTime > currentSimulationTime + simulationStepSize
            FMU -> FMU: Store message for usage in subsequent DoStep operation.
