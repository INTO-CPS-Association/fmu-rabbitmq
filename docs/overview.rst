Overview
=========
This page presents the RabbitMQ FMU and how to use it. Note, that terminology related to the Functional Mock-up Interface (FMI) and Advanced Message Queuing Protocol (AMQP) is not described in detail.

The purpose of RabbitMQ FMU is to provide a tool (and approach) to bring external data from a RabbitMQ Server (or any message broker implementing AMQP) into an FMI-based co-simulation.
It has certain quality attributes related to timeliness of messages and precision of data.

The overall approach is depicted in the diagram below.

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
            FMU -> FMU: Update output state with values from message
        else messageTimeStampInSimulationTime > currentSimulationTime + simulationStepSize
            FMU -> FMU: Store message for usage in subsequent DoStep operation.


Invoking the function :code:`fmi2EnterInitializationMode` on RabbitMQ FMU causes it to a create a topic exchange (if it does not exist), a binding key and a queue. Furthermore, it begins to process messages until all outputs of the FMU has been defined.
When all outputs has been defined, the time stamp of the latest message (*wct1* in the example) defines *simulation Time 0* and thus establishes a mapping between wall-clock time and simulation time. Any subsequent messages has *WCT1* subtracted in order to map them to *simulation Time*.

Invoking :code:`fmi2DoStep` on RabbitMQ FMU causes it to process messages and update its output state until there is a message with a time stamp defined by: :code:`messageTimeStampInSimulationTime>=currentSimulationTime+simulationStepSize`. Such a message is stored in order to use it for the subsequent :code:`fmi2DoStep` operation. The updating of output state are elaborated on in :doc:`data-handling`.
