import zipfile
import xml.etree.ElementTree as ET
import re
import argparse

"""
Utility for packing the Rabbitmq FMU with specific outputs and removing any old model specific data from the file

Author: Kenneth Lausdahl
"""


def create_fmu_with_outputs(path, new_path, names, verbose=False):
    with zipfile.ZipFile(path, 'r') as archive:
        md = archive.open('modelDescription.xml')

    tree = ET.parse(md)
    root = tree.getroot()

    # clean up the input FMU
    model_variables_node = tree.findall('ModelVariables')[0]
    model_outputs = tree.findall('ModelStructure/Outputs')[0]

    for n in model_variables_node.findall("ScalarVariable[@causality='output']"):
        model_variables_node.remove(n)

    for n in model_variables_node.findall("ScalarVariable[@causality='input']"):
        model_variables_node.remove(n)

    for n in model_outputs.findall("*"):
        model_outputs.remove(n)

    # generate new entries
    for idx, name in enumerate(names):
        # model_variables_node.append()
        output = ET.SubElement(model_variables_node, 'ScalarVariable')
        output.attrib['name'] = name
        output.attrib['valueReference'] = str(idx + 100)
        output.attrib['variability'] = 'continuous'
        output.attrib['causality'] = 'output'
        ET.SubElement(output, 'Real')

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

    python rabbitmq_fmu_configure.py -fmu rabbitmq.fmu -dest my-rabbitmq.fmu -output level valve

    """)
    options.add_argument("-fmu", dest="fmu", type=str, required=True, help='Path to the FMU file')
    options.add_argument("-dest", dest="dest", type=str, required=True,
                         help='Path to the destination FMU file that fill be generated')
    options.add_argument("-output", dest="output", action="extend", nargs="+", type=str, required=True,
                         help="A list of output names to be set in the FMU")
    options.add_argument("-v", "--verbose", required=False, help='Verbose', dest='verbose', action="store_true")

    args = options.parse_args()

    create_fmu_with_outputs(args.fmu, args.dest, args.output, verbose=args.verbose)


if __name__ == '__main__':
    main()
