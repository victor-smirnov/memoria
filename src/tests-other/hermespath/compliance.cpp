/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
**
** This file is part of the jmespath.cpp project which is distributed under
** the MIT License (MIT).
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#define CATCH_CONFIG_MAIN
#include "boost/fakeit.hpp"
#include <memoria/core/hermes/path/path.h>
#include "interpreter/interpreter.h"
#include <fstream>

using namespace memoria::hermes::path;

class ComplianceTestFixture
{
protected:
    void executeFeatureTest(const std::string& featureName,
                            bool passRvalue = false) const
    {
        Json testSuites = readTestSuites(featureName + ".json");
        for (auto& testSuite: testSuites)
        {
            const Json& document = testSuite["given"];
            const Json& testCases = testSuite["cases"];
            for (const auto& testCase: testCases)
            {
                auto expression = testCase["expression"]
                        .get_ref<const String&>();
                auto resultIt = testCase.find("result");
                if (resultIt != testCase.cend())
                {
                    if (!passRvalue)
                    {
                        testResult(expression, document, *resultIt);
                    }
                    else
                    {
                        testResult(expression, Json(document), *resultIt);
                    }
                }
                auto errorIt = testCase.find("error");
                if (errorIt != testCase.cend())
                {
                    if (!passRvalue)
                    {
                        testError(expression, document, *errorIt);
                    }
                    else
                    {
                        testError(expression, Json(document), *errorIt);
                    }
                }
                auto benchIt = testCase.find("bench");
                if (benchIt != testCase.cend())
                {
                    if (!passRvalue)
                    {
                        testBench(expression, document, *benchIt);
                    }
                    else
                    {
                        testBench(expression, Json(document), *benchIt);
                    }
                }
            }
        }
    }

    template <typename JsonT>
    void testResult(const std::string& expression,
                    JsonT&& document,
                    const Json& expectedResult) const
    {
        Json result;
        try
        {
            result = search(expression, std::forward<JsonT>(document));
        }
        catch(std::exception& exc)
        {
            FAIL("Exception: " + String(exc.what())
                 + "\nExpression: " + expression
                 + "\nExpected result: " + expectedResult.dump()
                 + "\nResult: " + result.dump());
        }

        if (result == expectedResult)
        {
            SUCCEED();
        }
        else
        {
            FAIL("Expression: " + expression
                 + "\nExpected result: " + expectedResult.dump()
                 + "\nResult: " + result.dump());
        }
    }

    template <typename JsonT>
    void testError(const std::string& expression,
                   JsonT&& document,
                   const std::string& expectedError) const
    {
        if (expectedError == "syntax")
        {
            REQUIRE_THROWS_AS(search(expression, std::forward<JsonT>(document)),
                              SyntaxError);
        }
        else if (expectedError == "invalid-value")
        {
            REQUIRE_THROWS_AS(search(expression, std::forward<JsonT>(document)),
                              InvalidValue);
        }
        else if (expectedError == "invalid-type")
        {
            REQUIRE_THROWS_AS(search(expression, std::forward<JsonT>(document)),
                              InvalidFunctionArgumentType);
        }
        else if (expectedError == "invalid-arity")
        {
            REQUIRE_THROWS_AS(search(expression, std::forward<JsonT>(document)),
                              InvalidFunctionArgumentArity);
        }
        else if (expectedError == "unknown-function")
        {
            REQUIRE_THROWS_AS(search(expression, std::forward<JsonT>(document)),
                              UnknownFunction);
        }
    }

    template <typename JsonT>
    void testBench(const std::string& expression,
                    JsonT&& document,
                    const std::string& benchType) const
    {
        if (benchType == "full")
        {
            Json result;
            try
            {
                result = search(expression, std::forward<JsonT>(document));
                SUCCEED();
            }
            catch(std::exception& exc)
            {
                FAIL("Exception: " + String(exc.what())
                     + "\nExpression: " + expression
                     + "\nBenchmark type: " + benchType
                     + "\nResult: " + result.dump());
            }
        }
        else if (benchType == "parse")
        {
            try
            {
                Expression parsedExpression{expression};
                SUCCEED();
            }
            catch(std::exception& exc)
            {
                FAIL("Exception: " + String(exc.what())
                     + "\nExpression: " + expression
                     + "\nBenchmark type: " + benchType);
            }
        }
    }

private:
    static const std::string s_absoluteDirPath;

    Json readTestSuites(const std::string& fileName) const
    {
        std::ifstream jsonFile;
        jsonFile.open(s_absoluteDirPath + "/" + fileName);
        REQUIRE(jsonFile.is_open());
        Json featureTest;
        jsonFile >> featureTest;
        jsonFile.close();
        return featureTest;
    }
};
const std::string ComplianceTestFixture::s_absoluteDirPath{
    JMESPATH_COMPLIANCETEST_DATA_PATH
};

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Identifiers lvalue",
                 "[identifiers][lvalue]")
{
    executeFeatureTest("identifiers");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Literals lvalue",
                 "[literals][lvalue]")
{
    executeFeatureTest("literal");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Basic expressions lvalue",
                 "[basic][lvalue]")
{
    executeFeatureTest("basic");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Boolean expressions lvalue",
                 "[boolean][lvalue]")
{
    executeFeatureTest("boolean");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Current node lvalue",
                 "[current][lvalue]")
{
    executeFeatureTest("current");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Escapes lvalue",
                 "[escape][lvalue]")
{
    executeFeatureTest("escape");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Filters lvalue",
                 "[filters][lvalue]")
{
    executeFeatureTest("filters");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Functions lvalue",
                 "[functions][lvalue]")
{
    executeFeatureTest("functions");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Index expressions lvalue",
                 "[indices][lvalue]")
{
    executeFeatureTest("indices");
}

TEST_CASE_METHOD(ComplianceTestFixture,
                 "Compliance/Multiselect expressions lvalue",
                 "[multiselect][lvalue]")
{
    executeFeatureTest("multiselect");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Pipe expressions lvalue",
                 "[pipe][lvalue]")
{
    executeFeatureTest("pipe");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Slice expressions lvalue",
                 "[slice][lvalue]")
{
    executeFeatureTest("slice");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Syntax lvalue",
                 "[syntax][lvalue]")
{
    executeFeatureTest("syntax");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Unicode lvalue",
                 "[unicode][lvalue]")
{
    executeFeatureTest("unicode");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Wildcard lvalue",
                 "[wildcard][lvalue]")
{
    executeFeatureTest("wildcard");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Benchmarks lvalue",
                 "[benchmarks][lvalue]")
{
    executeFeatureTest("benchmarks");
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Identifiers rvalue",
                 "[identifiers][rvalue]")
{
    executeFeatureTest("identifiers", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Literals rvalue",
                 "[literals][rvalue]")
{
    executeFeatureTest("literal", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Basic expressions rvalue",
                 "[basic][rvalue]")
{
    executeFeatureTest("basic", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Boolean expressions rvalue",
                 "[boolean][rvalue]")
{
    executeFeatureTest("boolean", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Current node rvalue",
                 "[current][rvalue]")
{
    executeFeatureTest("current", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Escapes rvalue",
                 "[escape][rvalue]")
{
    executeFeatureTest("escape", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Filters rvalue",
                 "[filters][rvalue]")
{
    executeFeatureTest("filters", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Functions rvalue",
                 "[functions][rvalue]")
{
    executeFeatureTest("functions", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Index expressions rvalue",
                 "[indices][rvalue]")
{
    executeFeatureTest("indices", true);
}

TEST_CASE_METHOD(ComplianceTestFixture,
                 "Compliance/Multiselect expressions rvalue",
                 "[multiselect][rvalue]")
{
    executeFeatureTest("multiselect", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Pipe expressions rvalue",
                 "[pipe][rvalue]")
{
    executeFeatureTest("pipe", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Slice expressions rvalue",
                 "[slice][rvalue]")
{
    executeFeatureTest("slice", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Syntax rvalue",
                 "[syntax][rvalue]")
{
    executeFeatureTest("syntax", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Unicode rvalue",
                 "[unicode][rvalue]")
{
    executeFeatureTest("unicode", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Wildcard rvalue",
                 "[wildcard][rvalue]")
{
    executeFeatureTest("wildcard", true);
}

TEST_CASE_METHOD(ComplianceTestFixture, "Compliance/Benchmarks rvalue",
                 "[benchmarks][rvalue]")
{
    executeFeatureTest("benchmarks", true);
}
