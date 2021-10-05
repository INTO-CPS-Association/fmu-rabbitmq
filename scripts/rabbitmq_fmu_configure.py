import zipfile
import xml.etree.ElementTree as ET
import re
import argparse

"""
Utility for packing the Rabbitmq FMU with specific outputs and removing any old model specific data from the file

Author: Kenneth Lausdahl
"""


def create_fmu_with_outputs(path, new_path, names, verbose=False, input_names=[]):
    with zipfile.ZipFile(path, 'r') as archive:
        md = archive.open('modelDescription.xml')

    tree = ET.parse(md)
    root = tree.getroot()

    # clean up the input FMU
    model_variables_node = tree.findall('ModelVariables')[0]
    model_outputs = tree.findall('ModelStructure/Outputs')[0]
    if tree.findall('ModelStructure/InitialUnknowns'):
        model_initunknowns = tree.findall('ModelStructure/InitialUnknowns')[0]
        for n in model_initunknowns.findall("*"):
            model_initunknowns.remove(n)

    for n in model_variables_node.findall("ScalarVariable[@causality='output']"):
        model_variables_node.remove(n)

    for n in model_variables_node.findall("ScalarVariable[@causality='input']"):
        model_variables_node.remove(n)

    for n in model_outputs.findall("*"):
        model_outputs.remove(n)



    index = 0
    # generate new entries
    for idx, (name,t,v) in enumerate(names):
        # model_variables_node.append()
        output = ET.SubElement(model_variables_node, 'ScalarVariable')
        output.attrib['name'] = name
        output.attrib['valueReference'] = str(idx + 100)
        # next two is combination 8
        output.attrib['variability'] = v  # from a real system
        output.attrib['causality'] = 'output'
        # The variable is calculated from other variables during initialization.
        # It is not allowed to provide a “start” value
        # This is of cause not completely true but is related to the server it is connected to
        output.attrib['initial'] = 'calculated'
        ET.SubElement(output, t)
        index = idx + 100

    if input_names:
        # generate new input entries
        index = index + 1
        # generate  new input entries
        for idx, (name,t,v) in enumerate(input_names):
            # model_variables_node.append()
            inputs = ET.SubElement(model_variables_node, 'ScalarVariable')
            inputs.attrib['name'] = name
            inputs.attrib['valueReference'] = str(index + idx)
            # next two is combination 8
            inputs.attrib['variability'] = v  # from a real system
            inputs.attrib['causality'] = 'input'
            # The variable is calculated from other variables during initialization.
            # It is not allowed to provide a “start” value
            # This is of cause not completely true but is related to the server it is connected to
            #input.attrib['initial'] = 'calculated'
            ET.SubElement(inputs, t)

    # updated the model structure with the new outputs
    for idx, item in enumerate(model_variables_node.findall("ScalarVariable")):
        if item.attrib['causality'] == 'output':
            ET.SubElement(model_outputs, 'Unknown').attrib['index'] = str(idx + 1)

    # we also need to set the initial unknowns
    model_initial_unknowns_list = tree.findall('ModelStructure/InitialUnknowns')
    if len(model_initial_unknowns_list) > 0:
        model_initial_unknowns = model_initial_unknowns_list[0]
    else:
        model_initial_unknowns = ET.SubElement(tree.findall('ModelStructure')[0], 'InitialUnknowns')

    for idx, item in enumerate(model_variables_node.findall("ScalarVariable")):
        if item.attrib['causality'] == 'output' and 'initial' in item.attrib and item.attrib['initial']:
            ET.SubElement(model_initial_unknowns, 'Unknown').attrib['index'] = str(idx + 1)

    # set default exchange
    for n in model_variables_node.findall("ScalarVariable[@name='config.routingkey']/String"):
        n.attrib['start'] = 'default'

    # print(ET.tostring(et, encoding="unicode", pretty_print=True))
    from xml.dom import minidom

    xmlstr = minidom.parseString(ET.tostring(root)).toprettyxml(indent="   ")
    pp_xml = re.sub(r'\n\s*\n', '\n', xmlstr)
    if verbose:
        print(pp_xml)

    # generate new FMU copying the input with a new model description
    with zipfile.ZipFile(path) as inzip, zipfile.ZipFile(new_path, "w") as outzip:
        # Iterate the input files
        for inzipinfo in inzip.infolist():
            # Read input file
            with inzip.open(inzipinfo) as infile:
                if "modelDescription.xml" in inzipinfo.filename:
                    outzip.writestr(inzipinfo.filename, pp_xml)
                else:
                    outzip.writestr(inzipinfo.filename, infile.read())


def parse_signals(signal: str):
    print(signal)
    if ('=' not in signal) and (',' not in signal):
        raise 'Signal triple invalid: %s' % signal

    name = signal[0:signal.index('=')]
    t = signal[signal.index('=') + 1:signal.index(',')]
    v = signal[signal.index(',') + 1:]
    return (name, t, v)


def main():
    options = argparse.ArgumentParser(prog="rabbitfmu", epilog="""

    python rabbitmq_fmu.py -fmu rabbitmq.fmu -dest my-rabbitmq.fmu -output level valve

    """)
    options.add_argument("-fmu", dest="fmu", type=str, required=True, help='Path to the FMU file')
    options.add_argument("-dest", dest="dest", type=str, required=True,
                         help='Path to the destination FMU file that fill be generated')
    options.add_argument("-output", dest="output", action="extend", nargs="+", type=str, required=True,
                         help="A list of output name type pairs (name=type,variability) to be set in the FMU. " +
                              "Valid types are: string, int, boolean, real")
    
    options.add_argument("-input", dest="input", action="extend", nargs="+", type=str, required=False,
                         help="A list of input name type pairs (name=type,variability) to be set in the FMU. " +
                              "Valid types are: String, Integer, Boolean, Real; variability: continuous, discrete")
    options.add_argument("-v", "--verbose", required=False, help='Verbose', dest='verbose', action="store_true")

    args = options.parse_args()

    if args.input:
        inputs = [parse_signals(s) for s in args.input] 
    else:
        inputs = []

    create_fmu_with_outputs(args.fmu, args.dest, [parse_signals(s) for s in args.output], verbose=args.verbose, input_names=inputs)


if __name__ == '__main__':
    main()
