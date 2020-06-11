Overview
=========
This page presents the RabbitMQ FMU and how to use it. Note, that terminology related to the Functional Mock-up Interface (FMI) and Advanced Message Queuing Protocol (AMQP) is is not described in detail.




The purpose of RabbitMQ FMU is to provide an approach to bringing external data from a RabbitMQ Server (or any message broker implementing AMQP) into an FMI-based co-simulation.
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
            FMU -> FMU: Update output values with values from message
        else messageTimeStampInSimulationTime > currentSimulationTime + simulationStepSize
            FMU -> FMU: Store message for usage in subsequent DoStep operation.


Invoking the function :code:`fmi2EnterInitializationMode` on RabbitMQ FMU causes it to a create a topic exchange (if it does not exist), a binding key and a queue. Furthermore, it begins to process messages until all outputs of the FMU has been defined.

When all outputs has been defined, the time stamp of the latest message (*wct1* in the example) defines *simulation Time 0* and thus establishes a mapping between wall-clock time and simulation time. Any subsequent messages has *WCT1* subtracted in order to map them to *simulation Time*.

Invoking :code:`fmi2DoStep` on RabbitMQ FMU causes it to process messages and update its output values until there is a message with a time stamp defined by: :code:`messageTimeStampInSimulationTime>=currentSimulationTime+simulationStepSize`.  Such a message is stored in order to use it for the subsequent :code:`fmi2DoStep` operation. The updating of output values are elaborated on in :doc:`data-handling`.

Creating a specification
------------------------
A specification can be written completely by hand if so desired, but Maestro2 also features a plugin system of `unfolding plugins` that can assist in creating a specification.
An unfolding plugin offer a function that can be called in the specification. If a specifiation makes use of unfolding plugins, 
then Maestro2 `unfolds` function calls to plugins with the behaviour of the function call, which is provided by the unfolding plugin.
As an example, consider a specification where a type conversion function from the TypeConverterPlugin is used in the initial specification passed to Maestro2:
:code:`convertBoolean2Real(booleanVariable, realVariable)`.
Maestro2 will then invoke the TypeConverterPlugin to get the unfolded MaBL code and replace the function call with the unfolded MaBL code provided by the TypeConverterPlugin::
    if( booleanVariable )
        {
            realVariable = 1.0;
        }
    else
        {
            realVariable = 0.0;
        }

Executing a specification
--------------------------
Maestro2 has an interpreter capable of executing a MaBL specification.
It is possible to define interpreter plugins that offer functions to be used in a MaBL specification. The interpreter will then invoke these functions during execution of the specification.
One such example is the CSV plugin, which writes values to a CSV file.

Outline of the Co-simulation Process with Maestro2
------------------------------------------------------
The sections outlines the process of how Maestro2 generates a MaBL specification and executes it.
The elements in the diagram are briefly described afterwards and extended upon elsewhere (TBD).

.. uml:: 
    
    title Co-Simulation with Maestro 2.
    hide footbox
    
    actor User #red
    participant Maestro
    participant "MablSpecification\nGenerator" as MablSpecGen
    participant "InitializePlugin  : \n IMaestroUnfoldPlugin" as InitializePlugin
    participant "FixedStepPlugin : \n IMaestroUnfoldPlugin" as FixedStepPlugin


    User -> Maestro: PerformCosimulation(environment.json, \nconfiguration.json, spec.mabl)
    Maestro -> MablSpecGen: GenerateSpecification(\nenvironment, configuration, spec)
    MablSpecGen -> InitializePlugin: unfold(environment, config, \nfunctionName, functionArguments)
    InitializePlugin -> MablSpecGen: unfoldedInitializeSpec
    MablSpecGen -> FixedStepPlugin: unfold(environment, \nfunctionName, functionArguments)
    FixedStepPlugin -> MablSpecGen: unfoldedFixedStepSpec
    MablSpecGen -> Maestro: unfoldedSpec
    Maestro -> Interpreter: Execute(unfoldedSpec)
    Interpreter -> "CSVPlugin : \n(TBD)\nIMaestroInterpreterPlugin": Log 
    Interpreter -> User: results


:environment.json: FMUs to use and the connections between instances of FMUs
:configuration.json: Configuration for the plugins.
:spec.mabl: Specification written in Maestro Base Language (MaBL). In this example, it contains two folded statements: :code:`initialize(arguments)` and :code:`fixedStep(arguments)` which are unfolded by plugins. This is furthermore described in the subsequent fields.
:MablSpecificationGenerator: Controls the process of creating a MaBL Specification from MaBL specifications and plugins.
:Unfold: Unfold refers to the process of unfolding. Unfolding is where a single statement is converted to multiple statements.
:IMaestroUnfoldPlugin: A plugin that is executed by the MablSpecificationGenerator during generation of a MaBL Specification. 
    A plugin that inherits from IMaestroUnfoldPlugin is capable of unfolding one or more MaBL statements.
:InitializePlugin \: IMaestroUnfoldPlugin: The initialize plugins unfolds the statementment :code:`initialize(arguments)` into MaBL statements that initializes the FMI2 instances passed via arguments
:FixedStepPlugin \: IMaestroUnfoldPlugin: The FixedStep plugins unfolds the statementment :code:`fixedStep(arguments)` into MaBL statements that creates the simulation statements required to execute a fixed step size algorithm based on the arguments. Note, it does not contain initialization. Initialization is taken care of by the InitializePlugin.
:UnfoldedSpec: A MaBL Specification that has been fully unfolded. 
:Interpreter: Can execute a MaBL Specification.
:IMaestroInterpreterPlugin: A plugin that is executed by the interpreter during the interpretation of a MaBL Specification.
:CSVPlugin \: IMaestroInterpreterPlugin: An interpreter plugin that logs values to a CSV file.
:results: A fully unfolded MaBL Specification and a CSV results file of the simulation.
