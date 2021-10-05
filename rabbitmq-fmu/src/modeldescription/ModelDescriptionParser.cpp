//
// Created by Kenneth Guldbrandt Lausdahl on 11/12/2019.
//

#include "ModelDescriptionParser.h"

#include <iostream>
#include "xercesc/dom/DOM.hpp"
#include "xercesc/dom/DOMDocument.hpp"
#include "xercesc/dom/DOMElement.hpp"
#include "xercesc/util/TransService.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"
#include <unistd.h>
#include <sstream>

using namespace std;
using namespace xercesc;
using SvType = ModelDescriptionParser::ScalarVariable::SvType;

char *getAttributeValue(const DOMNode *n, const char *name) {
    if (n->hasAttributes()) {
        auto valueAttribute = n->getAttributes()->getNamedItem(XMLString::transcode(name));
        if (valueAttribute != nullptr) {
            return XMLString::transcode(valueAttribute->getNodeValue());
        }
    }
    return nullptr;
}


map <string, ModelDescriptionParser::ScalarVariable> ModelDescriptionParser::parse(string path) {

    if (access(path.c_str(), 0) != 0) {
        throw "Invalid file path";
    }


    XMLPlatformUtils::Initialize();

    // create the DOM parser
    XercesDOMParser *parser = new XercesDOMParser;
    parser->
            setValidationScheme(XercesDOMParser::Val_Never);
    parser->parse(path.c_str());
    // get the DOM representation
    DOMDocument *doc = parser->getDocument();
    // get the root element
    DOMElement *root = doc->getDocumentElement();


    {
        DOMXPathResult *result = doc->evaluate(
                XMLString::transcode("/fmiModelDescription"),
                root,
                NULL,
                DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
                NULL);

        if (result->getNodeValue() == NULL) {
            cout << "There is no result for the provided XPath " <<
                 endl;

            throw std::runtime_error(std::string("Not a valid model description: ") + path);
        } else {

            auto version = XMLString::transcode(result->getNodeValue()->getAttributes()->getNamedItem(
                    XMLString::transcode("fmiVersion"))->getNodeValue());
//            cout << version << endl;
            auto versionString = string(version);
            XMLString::release(&version);
//            cout << versionString << endl;
            if (string("2.0").compare(versionString)) {
                throw std::runtime_error(std::string("Model description version not supported: ") + versionString);
            }

        }
    }


    // evaluate the xpath
    DOMXPathResult *result = doc->evaluate(
            XMLString::transcode("/fmiModelDescription/ModelVariables/ScalarVariable"),
            root,
            NULL,
            DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
            NULL);


    map <string, ModelDescriptionParser::ScalarVariable> svNameRefMap;

    XMLSize_t nLength = result->getSnapshotLength();
    for (XMLSize_t i = 0; i < nLength; i++) {
        result->snapshotItem(i);

        const DOMNode *n = result->getNodeValue();

        if (n == NULL) {
            cout << "There is no result for the provided XPath " <<
                 endl;
        } else {

            auto nameTagValue = XMLString::transcode(
                    n->getAttributes()->getNamedItem(XMLString::transcode("name"))->getNodeValue());

            auto key = string(nameTagValue);
            XMLString::release(&nameTagValue);

            auto valueTag = XMLString::transcode(
                    n->getAttributes()->getNamedItem(XMLString::transcode("valueReference"))->getNodeValue());

            stringstream strValue;
            strValue << valueTag;
            XMLString::release(&valueTag);

            unsigned int intValue;
            strValue >> intValue;

            ModelDescriptionParser::ScalarVariable sv;
            sv.name = key;
            sv.valueReference = intValue;

            auto causality = getAttributeValue(n, "causality");
            bool isOutput = string(causality) == "output";
            bool isInput = string(causality) == "input";
            XMLString::release(&causality);

            if (n->hasChildNodes()) {
                auto childrens = n->getChildNodes();
                for (XMLSize_t childIndex = 0; childIndex < childrens->getLength(); childIndex++) {
                    auto child = childrens->item(childIndex);
                    auto tmp = XMLString::transcode(child->getNodeName());
                    auto typeName = string(tmp);
                    XMLString::release(&tmp);

                    if (string("#text") == typeName) {
                        continue;
                    }

                    SvType type = SvType::Real;
                    if (string("Real") == typeName) {
                        type = SvType::Real;
                    } else if (string("Integer") == typeName) {
                        type = SvType::Integer;
                    } else if (string("Boolean") == typeName) {
                        type = SvType::Boolean;
                    } else if (string("String") == typeName) {
                        type = SvType::String;
                    }

                    sv.type = type;
                    sv.output = isOutput;
                    sv.input = isInput;

                    if (isOutput) {
                        //set default values
                        switch (type) {
                            case SvType::Real:
                                sv.setReal(0);
                                break;
                            case SvType::Integer:
                                sv.setInt(0);
                                break;
                            case SvType::Boolean:
                                sv.setBool(0);
                                break;
                            case SvType::String:
                                sv.setString(string(""));
                                break;
                        }
                    }


                    auto stringVal = getAttributeValue(child, "start");

                    if (stringVal != nullptr) {

                        stringstream strValue;

                        switch (type) {
                            case SvType::Real: {
                                strValue << stringVal;
                                double dVal;
                                strValue >> dVal;
                                sv.setReal(dVal);
                            }
                                break;
                            case SvType::Integer: {
                                strValue << stringVal;
                                int iVal;
                                strValue >> iVal;
                                sv.setInt(iVal);
                            }
                                break;
                            case SvType::Boolean: {
                                strValue << stringVal;
                                bool dVal;
                                strValue >> dVal;
                                sv.setBool(dVal);
                            }
                                break;
                            case SvType::String: {
                                sv.setString(string(stringVal));
                            }
                                break;
                        }

                        XMLString::release(&stringVal);
                    }
                }
            }
            svNameRefMap[key] = sv;
        }
    }


    XMLPlatformUtils::Terminate();

    return svNameRefMap;
}

string ModelDescriptionParser::extractToolVersion(string path) {

    if (access(path.c_str(), 0) != 0) {
        throw "Invalid file path";
    }

    XMLPlatformUtils::Initialize();

    // create the DOM parser
    XercesDOMParser *parser = new XercesDOMParser;
    parser->
            setValidationScheme(XercesDOMParser::Val_Never);
    parser->parse(path.c_str());
    // get the DOM representation
    DOMDocument *doc = parser->getDocument();
    // get the root element
    DOMElement *root = doc->getDocumentElement();

    {
        DOMXPathResult *result = doc->evaluate(
                XMLString::transcode("/fmiModelDescription"),
                root,
                NULL,
                DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
                NULL);

        if (result->getNodeValue() == NULL) {
            cout << "There is no result for the provided XPath " <<
                 endl;

            throw std::runtime_error(std::string("Not a valid model description: ") + path);
        } 
        else {
            auto version = XMLString::transcode(result->getNodeValue()->getAttributes()->getNamedItem(
                    XMLString::transcode("generationTool"))->getNodeValue());
            auto toolVersion = string(version);
            return toolVersion;
        }
    }

}

DataPoint ModelDescriptionParser::extractDataPoint(map <string, ScalarVariable> svs) {
    DataPoint dp;

    for (const auto &pair : svs) {

        auto sv = pair.second;

        if (!sv.hasStartValue) {
            continue;
        }

        switch (sv.type) {
            case SvType::Integer:
                dp.integerValues[sv.valueReference] = sv.i_value;
                break;
            case SvType::Real:
                dp.doubleValues[sv.valueReference] = sv.d_value;
                break;
            case SvType::Boolean:
                dp.booleanValues[sv.valueReference] = sv.b_value;
                break;
            case SvType::String:
                dp.stringValues[sv.valueReference] = sv.s_value;
                break;
        }
    }
    return dp;
}