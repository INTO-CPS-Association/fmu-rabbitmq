import zipfile
import xml.etree.ElementTree as ET
import re
import sys
import argparse

"""
Utility for packing the Rabbitmq FMU with specific outputs and removing any old model specific data from the file

Author: Kenneth Lausdahl
"""


def create_fmu_with_outputs(path, new_path, names, verbose=False, input_names=[], input_types=[], output_types=[], input_var=[], output_var=[]):
    with zipfile.ZipFile(path, 'r') as archive:
        md = archive.open('modelDescription.xml')

    tree = ET.parse(md)
    root = tree.getroot()

    # clean up the input FMU
    model_variables_node = tree.findall('ModelVariables')[0]
    model_outputs = tree.findall('ModelStructure/Outputs')[0]
    model_initunknowns = tree.findall('ModelStructure/InitialUnknowns')[0]

    for n in model_variables_node.findall("ScalarVariable[@causality='output']"):
        model_variables_node.remove(n)

    for n in model_variables_node.findall("ScalarVariable[@causality='input']"):
        model_variables_node.remove(n)

    for n in model_outputs.findall("*"):
        model_outputs.remove(n)

    for n in model_initunknowns.findall("*"):
        model_initunknowns.remove(n)

    # keep track of index
    index = 0
    # generate new entries
    for idx, name in enumerate(names):
        # model_variables_node.append()
        output = ET.SubElement(model_variables_node, 'ScalarVariable')
        output.attrib['name'] = name
        output.attrib['valueReference'] = str(idx + 100)
        if output_var:
            output.attrib['variability'] = output_var[idx]
        else:
            output.attrib['variability'] = 'continuous'
        output.attrib['causality'] = 'output'
        if output_types:
            ET.SubElement(output, output_types[idx])
        else:
            ET.SubElement(output, 'Real')
        index = idx + 100

    # generate new input entries
    index = index + 1
    if input_names:
        for idx, name in enumerate(input_names):
            # model_variables_node.append()
            inputsv = ET.SubElement(model_variables_node, 'ScalarVariable')
            inputsv.attrib['name'] = name
            inputsv.attrib['valueReference'] = str(idx + index)
            if input_var:
                inputsv.attrib['variability'] = input_var[idx]
            else:
                inputsv.attrib['variability'] = 'continuous'
            inputsv.attrib['causality'] = 'input'

            if input_types:
                ET.SubElement(inputsv, input_types[idx])
            else:
                ET.SubElement(inputsv, 'Real')

    # updated the model structure with the new outputs
    for idx, item in enumerate(model_variables_node.findall("ScalarVariable")):
        if item.attrib['causality'] == 'output':
            ET.SubElement(model_outputs, 'Unknown').attrib['index'] = str(idx + 1)

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


def main():
    options = argparse.ArgumentParser(prog="rabbitmq_fmu_configure.py", epilog="""

    python rabbitmq_fmu_configure.py -fmu rabbitmq.fmu -dest my-rabbitmq.fmu -output level valve -input command1 command2

    """)
    options.add_argument("-fmu", dest="fmu", type=str, required=True, help='Path to the FMU file')
    options.add_argument("-dest", dest="dest", type=str, required=True,
                         help='Path to the destination FMU file that fill be generated')
    options.add_argument("-output", dest="output", action="extend", nargs="+", type=str, required=True,
                         help="A list of output names to be set in the FMU, generated with default variability=continuous and type=Real")
    options.add_argument("-input", dest="input", action="extend", nargs="+", type=str, required=False,
                         help="A list of input names to be set in the FMU, optional, generated with default variability=continuous and type=Real")
    options.add_argument("-inputTypes", dest="inputTypes", action="extend", nargs="+", type=str, required=False,
                         help="A list of input types")
    options.add_argument("-outputTypes", dest="outputTypes", action="extend", nargs="+", type=str, required=False,
                         help="A list of output types")

    options.add_argument("-inputVar", dest="inputVar", action="extend", nargs="+", type=str, required=False,
                         help="A list of input variabilities")
    options.add_argument("-outputVar", dest="outputVar", action="extend", nargs="+", type=str, required=False,
                         help="A list of output variabilities")

    options.add_argument("-v", "--verbose", required=False, help='Verbose', dest='verbose', action="store_true")

    args = options.parse_args()
    # if types and variabilities are given, assert that the length of those arrays same as the outputs/inputs
    if args.outputTypes and not len(args.outputTypes)==len(args.output):
        print("list of output types should have the same length as the list of outputs")
        print("exiting")
        sys.exit()
    if args.inputTypes and not len(args.inputTypes)==len(args.input):
        print("list of input types should have the same length as the list of inputs")
        print("exiting")
        sys.exit()
    if args.outputVar and not len(args.outputVar)==len(args.output):
        print("list of output variabilities should have the same length as the list of inputs")
        print("exiting")
        sys.exit()
    if args.inputVar and not len(args.inputVar)==len(args.input):
        print("list of input variabilities should have the same length as the list of inputs")
        print("exiting")
        sys.exit()

    create_fmu_with_outputs(args.fmu, args.dest, args.output, verbose=args.verbose, input_names=args.input, 
                input_types=args.inputTypes, output_types=args.outputTypes, input_var=args.inputVar, output_var=args.outputVar)


if __name__ == '__main__':
    main()
