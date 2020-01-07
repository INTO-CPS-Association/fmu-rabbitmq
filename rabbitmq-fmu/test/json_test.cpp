#include "gtest/gtest.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>


using namespace std;
using namespace rapidjson;

namespace {
    TEST(JsonTest, Basic
    ) {
        // 1. Parse a JSON string into DOM.
        const char *json = "{\"project\":\"rapidjson\",\"stars\":10}";
        Document d;
        d.Parse(json);

        // 2. Modify it by DOM.
        Value &s = d["stars"];
        s.SetInt(s.GetInt() + 13);

        int k = s.GetInt();

        // 3. Stringify the DOM
        StringBuffer buffer;
        Writer <StringBuffer> writer(buffer);
        d.Accept(writer);

        // Output {"project":"rapidjson","stars":11}
        std::cout << buffer.GetString() << std::endl;
    }
}