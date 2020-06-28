Overview
=========
This page presents a rough overview of the RabbitMQ FMU (RMQFMU). Note, that terminology related to the Functional Mock-up Interface 2.0 (FMI) and RabbitMQ is not described in detail.
The internal workings are described in detail in :doc:`data-handling` and a user manual is available in :doc:`user-manual`. It is adviced to read the user manual first.

The purpose of RMQFMU is to provide a tool (and approach) for briging external data from a RabbitMQ Server into an FMI-based co-simulation.
It has certain properties related to timeliness of messages, timeout and precision of data.

The overall approach is depicted in the diagram below.

.. uml::

    title Time Handling for a Single Message RabbitMQ
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
    server -> FMU: "Publish message with timestamp of WCT1 and variable values."
    note over FMU: "Rabbit FMU unblocks and sets Simulation Time 0 = WCT1 and sets its output state"

    == Simulation Loop until endtime ==
    exec -> FMU: Fmi2DoStep(currentSimulationTime, simulaionStepSize)
        loop FMU process messages until messageTimeStampInSimulationTime >= currentSimulationTime + simulationStepSize
        server -> FMU: Message
        alt messageTimeStampInSimulationTime <= currentSimulationTime+simulationStepSize
            FMU -> FMU: Update output state with values from message
        else messageTimeStampInSimulationTime > currentSimulationTime + simulationStepSize
            FMU -> FMU: Store message for usage in subsequent DoStep operation.


Invoking the function :code:`fmi2EnterInitializationMode` on RabbitMQ FMU causes it to a create a topic exchange (if it does not exist), a binding key and a queue. Furthermore, it begins to process messages until all outputs of the FMU has been defined.
When all outputs has been defined, the time stamp of the latest message (*wct1* in the example) defines *simulation Time 0* and thus establishes a mapping between wall-clock time (WCT) and simulation time. Any subsequent messages has *WCT1* subtracted in order to map them to *simulation Time*.

Invoking :code:`fmi2DoStep` on RabbitMQ FMU causes it to process messages and update its output state until there is a message with a time stamp defined by: :code:`messageTimeStampInSimulationTime >= currentSimulationTime+simulationStepSize`. Such a message is stored in order to use it for the subsequent :code:`fmi2DoStep` operation.
