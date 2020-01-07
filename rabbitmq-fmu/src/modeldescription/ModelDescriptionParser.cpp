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


#include <iostream>
#include <sstream>

using namespace std;
using namespace xercesc;
using SvType = ModelDescriptionParser::ScalarVariable::SvType;

char *getAttributeValue(const DOMNode *n, const char *name) {
    if (n->hasAttributes()) {
        auto valueAttribute = n->getAttributes()->getNamedItem(XMLString::transcode(name));
        if (valueAttribute != nullptr) {
            return (char *) TranscodeToStr(valueAttribute->getNodeValue(), "utf-8").str();
        }
    }
    return nullptr;
}


map<string, ModelDescriptionParser::ScalarVariable> ModelDescriptionParser::parse(string path) {

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

            auto version = TranscodeToStr(result->getNodeValue()->getAttributes()->getNamedItem(
                    XMLString::transcode("fmiVersion"))->getNodeValue(),
                                          "ascii").str();
//            cout << version << endl;
            auto versionString = string((char *) version);
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


    map<string, ModelDescriptionParser::ScalarVariable> svNameRefMap;

    XMLSize_t nLength = result->getSnapshotLength();
    for (XMLSize_t i = 0; i < nLength; i++) {
        result->snapshotItem(i);

        const DOMNode *n = result->getNodeValue();

        if (n == NULL) {
            cout << "There is no result for the provided XPath " <<
                 endl;
        } else {
//            cout << TranscodeToStr(n->getAttributes()->getNamedItem(XMLString::transcode("name"))->getNodeValue(),
//                                   "ascii").str() << endl;

            auto nameTagValue = TranscodeToStr(
                    n->getAttributes()->getNamedItem(XMLString::transcode("name"))->getNodeValue(),
                    "ascii").str();

            auto key = string((char *) nameTagValue);

            auto valueTag = TranscodeToStr(
                    n->getAttributes()->getNamedItem(XMLString::transcode("valueReference"))->getNodeValue(),
                    "ascii").str();

            stringstream strValue;
            strValue << valueTag;

            unsigned int intValue;
            strValue >> intValue;

            ModelDescriptionParser::ScalarVariable sv;
            sv.name = key;
            sv.valueReference = intValue;

            bool isOutput = string(getAttributeValue(n, "causality")) == "output";

            if (n->hasChildNodes()) {
                auto childrens = n->getChildNodes();
                for (XMLSize_t childIndex = 0; childIndex < childrens->getLength(); childIndex++) {
                    auto child = childrens->item(childIndex);
                    auto typeName = string((char *) TranscodeToStr(child->getNodeName(), "utf-8").str());

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


                    }
                }
            }
            svNameRefMap[key] = sv;
        }
    }


    XMLPlatformUtils::Terminate();

    return svNameRefMap;
}

DataPoint ModelDescriptionParser::extractDataPoint(map<string, ScalarVariable> svs) {
    DataPoint dp;

    dp.time = 0;
    for (const auto &pair : svs) {

        auto sv = pair.second;
//        cout << sv.valueReference << "->" << sv.name << " type " << sv.type << " has start value " << sv.hasStartValue
//             << endl;

        if (!sv.hasStartValue) {
            continue;
        }

        switch (sv.type) {
            case SvType::Integer:
//                cout << sv.valueReference << "->" << sv.i_value << endl;
                dp.integerValues[sv.valueReference] = sv.i_value;
                break;
            case SvType::Real:
//                cout << sv.valueReference << "->" << sv.d_value << endl;
                dp.doubleValues[sv.valueReference] = sv.d_value;
                break;
            case SvType::Boolean:
//                cout << sv.valueReference << "->" << sv.b_value << endl;
                dp.booleanValues[sv.valueReference] = sv.b_value;
                break;
            case SvType::String:
//                cout << sv.valueReference << "->" << sv.s_value << endl;
                dp.stringValues[sv.valueReference] = sv.s_value;
                break;
        }
    }
    return dp;
}