#include "gtest/gtest.h"

#include <iostream>
#include "xercesc/dom/DOM.hpp"
#include "xercesc/dom/DOMDocument.hpp"
#include "xercesc/dom/DOMElement.hpp"
#include "xercesc/util/TransService.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"

#include <xercesc/sax/HandlerBase.hpp>

using namespace xercesc;
using namespace std;

namespace {

    void test1() {
        XMLPlatformUtils::Initialize();

        // create the DOM parser
        XercesDOMParser *parser = new XercesDOMParser;
        parser->setValidationScheme(XercesDOMParser::Val_Never);
        ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
        parser->setErrorHandler(errHandler);
        try {
            parser->parse("sample.xml");
            // get the DOM representation
            DOMDocument *doc = parser->getDocument();
            // get the root element
            DOMElement *root = doc->getDocumentElement();
            // evaluate the xpath
            DOMXPathResult *result = doc->evaluate(
            XMLString::transcode("/root/ApplicationSettings"),
            root,
            NULL,
            DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
            NULL);

            if (result->getNodeValue() == NULL) {
            cout << "There is no result for the provided XPath " <<
            endl;
            } else {
            cout << TranscodeToStr(result->getNodeValue()->getFirstChild()->getNodeValue(), "ascii").str() << endl;

            }

            XMLPlatformUtils::Terminate();

        }
        catch (const XMLException& toCatch) {
            char* message = XMLString::transcode(toCatch.getMessage());
            cout << "Exception message is: \n"
                    << message << "\n";
            XMLString::release(&message);
        }
        catch (const DOMException& toCatch) {
            char* message = XMLString::transcode(toCatch.msg);
            cout << "Exception message is: \n"
                    << message << "\n";
            XMLString::release(&message);
        }

    }

    void test2() {
        XMLPlatformUtils::Initialize();

        // create the DOM parser
        XercesDOMParser *parser = new XercesDOMParser;
        parser->
                setValidationScheme(XercesDOMParser::Val_Never);
        parser->parse("modelDescription.xml");
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
            } else {
                cout << TranscodeToStr(result->getNodeValue()->getAttributes()->getNamedItem(XMLString::transcode("fmiVersion"))->getNodeValue(),
                                       "ascii").str() << endl;
            }
        }


        // evaluate the xpath
        DOMXPathResult *result = doc->evaluate(
                XMLString::transcode("/fmiModelDescription/ModelVariables/ScalarVariable"),
                root,
                NULL,
                DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
                NULL);

        cout << "got result " <<
             endl;

        XMLSize_t nLength = result->getSnapshotLength();
        for(XMLSize_t i = 0; i < nLength; i++)
        {
            result->snapshotItem(i);

          const DOMNode*n = result->getNodeValue();

            if (n == NULL) {
                cout << "There is no result for the provided XPath " <<
                     endl;
            } else {
                cout << TranscodeToStr(n->getAttributes()->getNamedItem(XMLString::transcode("name"))->getNodeValue(),
                                       "ascii").str() << endl;
            }
        }



        XMLPlatformUtils::Terminate();
    }

    TEST(XmlTest, Negative
    ) {

        test1();


        EXPECT_EQ(1, 1);

    }

    TEST(XmlTest, modeldescription
    ) {


        try {
            test2();
            // code that could cause exception
        }
        catch (const std::exception &exc) {
            // catch anything thrown within try block that derives from std::exception
            std::cerr << exc.what();
        }

        EXPECT_EQ(1, 1);

    }
}