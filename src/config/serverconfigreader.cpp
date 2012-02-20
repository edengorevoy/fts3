/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and 
limitations under the License. */

/** \file serverconfigreader.h Implementation of FTS3 server config reader. */

#include "serverconfigreader.h"
#include "common/error.h"

#include <iostream>
#include <fstream>

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
    #include <boost/algorithm/string/find.hpp>
    #include <cstdio>
#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ---------------------------------------------------------------------- */

FTS3_CONFIG_NAMESPACE_START

// Default config values
#define FTS3_CONFIG_SERVERCONFIG_PORT_DEFAULT 8080
#define FTS3_CONFIG_SERVERCONFIG_IP_DEFAULT "localhost"
#define FTS3_CONFIG_SERVERCONFIG_THREADNUM_DEFAULT 2000
#define FTS3_CONFIG_SERVERCONFIG_TRANSFERLOGFIRECTOTY_DEFAULT "/var/log/fts3"
#define FTS3_CONFIG_SERVERCONFIG_CONFIGFILE_DEFAULT "/etc/sysconfig/fts3config"

/* ---------------------------------------------------------------------- */

po::options_description ServerConfigReader::_defineGenericOptions()
{
	po::options_description generic("Generic options");
	generic.add_options()
	    ("help,h", "Display this help page")
        ("version,v", "Display server version")
        ("no-daemon,n", "Do not daemonize")

        (
            "configfile,c",  
            po::value<std::string>( &(_vars["configfile"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_CONFIGFILE_DEFAULT),
            "FTS3 server config file"
        );

    return generic;
}

/* ---------------------------------------------------------------------- */

po::options_description ServerConfigReader::_defineConfigOptions()
{
	po::options_description config("Configuration");

    config.add_options()
	    (   
            "Port,p", 
            po::value<int>()->default_value(FTS3_CONFIG_SERVERCONFIG_PORT_DEFAULT), 
            "Listening port"
        )

	    (
            "IP,i", 
            po::value<std::string>( &(_vars["IP"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_IP_DEFAULT), 
            "IP address that the server is bound to"
        )

	    (
            "DbConnectString,s", 
            po::value<std::string>( &(_vars["DbConnectString"]) )->default_value(""),
            "Connect string for the used database account"
        )

	    (
            "DbUserName,u", 
            po::value<std::string>( &(_vars["DbUserName"]) )->default_value(""), 
            "Database account user name"
        )
	    
        (
            "DbPassword,w", 
            po::value<std::string>( &(_vars["DbPassword"]) )->default_value(""),
            "Database account password"
        )
	    
        (
            "TransferLogDirectory,l", 
            po::value<std::string>( &(_vars["TransferLogDirectory"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_TRANSFERLOGFIRECTOTY_DEFAULT),
            "Directory where the individual transfer logs are written"
        );

    return config;
}

/* ---------------------------------------------------------------------- */

po::options_description ServerConfigReader::_defineHiddenOptions()
{
	po::options_description hidden("Hidden options");

    hidden.add_options()
        (
            "ThreadNum,t", 
            po::value<int>()->default_value(FTS3_CONFIG_SERVERCONFIG_THREADNUM_DEFAULT),
            "Number of worker threads."
        );

    return hidden;
}

/* ========================================================================== */

/** Read command line option - the real thing. */
struct ReadCommandLineOptions_SystemTraits 
{
    static void exit(const int aVal)
    {
        ::exit(aVal);
    }

    /* ---------------------------------------------------------------------- */
    
    static std::ostream& stream()
    {
        return std::cout;
    }

    /* ---------------------------------------------------------------------- */
    
    static void processVariables
    (
        ServerConfigReader& aReader
    )
    {
        aReader.storeValuesAsStrings ();
    }
};

/* ========================================================================== */

/** Read config file - the real thing. */
struct ReadConfigFile_SystemTraits
{
    static boost::shared_ptr<std::istream> getStream (const std::string& aName)
    {
        boost::shared_ptr<std::istream> in 
        (
            dynamic_cast<std::istream*> (new std::ifstream(aName.c_str()))
        );

        if (!(*in))
        {
            std::stringstream msg;
            msg << "Error opening file " << aName; 
            FTS3_COMMON_EXCEPTION_THROW ( FTS3_COMMON_NAMESPACE::Err_System (msg.str()) );
        }

        return in;
    }

    /* ---------------------------------------------------------------------- */
    
    static void processVariables
    (
        ServerConfigReader& reader
    )
    {
        reader.storeValuesAsStrings();
    }
};

/* ---------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_readConfigFile_SystemTraits, 
    ReadConfigFile_SystemTraits
)
{
    // Test non-existing file opening
    BOOST_CHECK_EXCEPTION 
    (
        getStream("/atyala/patyala/thisfile-doesnot_exis"),
        FTS3_COMMON_NAMESPACE::Err_System,
        fts3_unittest_always_true
    );

    // Test opening existing file
    boost::shared_ptr<std::istream> in = getStream ("/etc/group");
    BOOST_CHECK (in.get());
    BOOST_CHECK (*in);
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ========================================================================== */

ServerConfigReader::type_return ServerConfigReader::operator() (int argc, char** argv)
{

	po::options_description generic = _defineGenericOptions();
	po::options_description config = _defineConfigOptions();
	po::options_description hidden = _defineHiddenOptions();

    // Option group in the command line
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config).add(hidden);
    _readCommandLineOptions<ReadCommandLineOptions_SystemTraits> (argc, argv, cmdline_options);     
   
    // Option group in config file
    po::options_description config_file_options;
    config_file_options.add(config).add(hidden);
    _readConfigFile<ReadConfigFile_SystemTraits> (config_file_options);

    return _vars;
}

/* ------------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST

/** This test checks if command line options really change default values */
BOOST_FIXTURE_TEST_CASE (Common__ServerConfigReader_functionOperator_default, ServerConfigReader)
{
    static const int argc = 3; 
    char *argv[argc];  
    argv[0] = const_cast<char*> ("executable");
    argv[1] = const_cast<char*> ("--configfile=/dev/null");
    argv[2] = const_cast<char*> ("--Port=7823682");
    (*this)(argc, argv);
    BOOST_CHECK_EQUAL (_vars["Port"], std::string("7823682"));
}

/* ------------------------------------------------------------------------- */

/** Check if you can specify all the options in a config file */
BOOST_FIXTURE_TEST_CASE (Common__ServerConfigReader_functionOperator_fromfile, ServerConfigReader)
{
    // Open a temporary file
    char filename[L_tmpnam];
    std::tmpnam(filename);
    std::ofstream file(filename);
    // Write a temporary config file
    std::string f_intval = "32234";
    std::string f_str = "randomval";
    file << "Port=" << f_intval << std::endl;
    file << "IP=" << f_str << std::endl;
    file << "DbConnectString=" << f_str << std::endl;
    file << "DbUserName=" << f_str << std::endl;
    file << "DbPassword=" << f_str << std::endl;
    file << "TransferLogDirectory=" << f_str << std::endl;
    file << "ThreadNum=" << f_intval << std::endl;
    file.close(); 
    // Read from the file
    static const int argc = 2; 
    char *argv[argc];  
    argv[0] = const_cast<char*> ("executable");
    std::string confpar = std::string("--configfile=") + filename;
    argv[1] = const_cast<char*> (confpar.c_str());
    (*this)(argc, argv);
    // Do the checks
    BOOST_CHECK_EQUAL (_vars["Port"], f_intval);
    BOOST_CHECK_EQUAL (_vars["ThreadNum"], f_intval);
    BOOST_CHECK_EQUAL (_vars["IP"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbConnectString"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbUserName"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbPassword"], f_str);
    BOOST_CHECK_EQUAL (_vars["TransferLogDirectory"], f_str);

    std::remove(filename);
}

#endif // FTS3_COMPILE_WITH_UNITTEST

/* ========================================================================== */

#ifdef FTS3_COMPILE_WITH_UNITTEST

/** Traits to test _readCommandLineOptions */
struct readCommandLineOptions_TestTraits
{
    static void exit(const int)
    {
        exitCalled = true;
    }

    /* ---------------------------------------------------------------------- */
    
    static void processVariables
    (
        ServerConfigReader&
    )
    {
        processVariablesCalled = true;
    }
    
    /* ---------------------------------------------------------------------- */
    
    static void reset()
    {
        processVariablesCalled = false;
        exitCalled = false;
        strstream.str("");
    }

    /* ---------------------------------------------------------------------- */
    
    static std::ostream& stream()
    {
        return strstream;
    }

    /* ---------------------------------------------------------------------- */

    static bool processVariablesCalled;
    static bool exitCalled;
    static std::stringstream strstream;
};

bool readCommandLineOptions_TestTraits::processVariablesCalled;
bool readCommandLineOptions_TestTraits::exitCalled;
std::stringstream readCommandLineOptions_TestTraits::strstream;

/* ---------------------------------------------------------------------- */

struct TestServerConfigReader : public ServerConfigReader
{
    TestServerConfigReader()
    {
        argv[0] = const_cast<char*> ("executable");
        testDesc.add_options()("help,h", "Description");
        testDesc.add_options()("version", "Description");
        testDesc.add_options()("no-daemon,n", "Description");
        testDesc.add_options()("other", po::value<std::string>(), "Description");
        testDesc.add_options()("intpar", po::value<int>(), "Description");
    }

    /* ---------------------------------------------------------------------- */
    
    void setupParameters
    (
        const std::string& aOption
    )
    {
        readCommandLineOptions_TestTraits::reset();
        argv[1] = const_cast<char*> (aOption.c_str());
    }

    /* ---------------------------------------------------------------------- */
    
    /** This test checks if:
     *    - --help option recognized
     *    - help message displayed
     *    - program exits
     */
    void do_helpTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        BOOST_CHECK (readCommandLineOptions_TestTraits::exitCalled);
        std::string f_helpMessage("-h [ --help ]         Description");
        std::string displayedText = readCommandLineOptions_TestTraits::strstream.str();
        bool contained = boost::find_first (displayedText, f_helpMessage);
        BOOST_CHECK (contained);
    }

    /* ---------------------------------------------------------------------- */
    
    /** This test checks if:
     *    - version option displays FTS3 version string
     *    - program exits
     */
    void do_versionTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        BOOST_CHECK (readCommandLineOptions_TestTraits::exitCalled);
        std::string f_versionMessage(FTS3_SERVER_VERSION);
        std::string displayedText = readCommandLineOptions_TestTraits::strstream.str();
        bool contained = boost::find_first (displayedText, f_versionMessage);
        BOOST_CHECK (contained);
    }
   
    /* ---------------------------------------------------------------------- */
    
    /** This test checks if:
     *    - Any other options than helo or version calls provessVariables
     *    - program does not exit
     */
    void do_othersTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        BOOST_CHECK ( ! readCommandLineOptions_TestTraits::exitCalled);
        BOOST_CHECK ( readCommandLineOptions_TestTraits::processVariablesCalled);
    }

    /* ---------------------------------------------------------------------- */
    
    /** This test checks the effect of nodaemon flags. */
    void do_noDaemonSpecifiedTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        BOOST_CHECK_EQUAL (_vars["no-daemon"], std::string("1"));
    }

    /* ---------------------------------------------------------------------- */
    
    /** This test checks the effect of nodaemon flags. */
    void do_noDaemonNotSpecifiedTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        BOOST_CHECK_EQUAL (_vars["no-daemon"], std::string("0"));
    }

protected:

    /* ---------------------------------------------------------------------- */
    
    static const int argc = 2;
    char *argv[argc];
    po::options_description testDesc;
};

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_readCommandLineOptions_help_long, 
    TestServerConfigReader
)
{
    // Test executing long help option
    setupParameters ("--help" );
    do_helpTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_readCommandLineOptions_help_short, 
    TestServerConfigReader
)
{    
    // Test executing short help option
    setupParameters ("-h" );
    do_helpTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_readCommandLineOptions_version, 
    TestServerConfigReader
)
{
    // Test executing "version"
    setupParameters("--version");
    do_versionTest();
}
    
/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_readCommandLineOptions_other, 
    TestServerConfigReader
)
{
    // Test executing "other" parameter. Test fixture requires paremeter to 
    // other!
    setupParameters("--other=value");
    do_othersTest();
} 

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_readCommandLineOptions_nodaemon_long, 
    TestServerConfigReader
)
{
    setupParameters ("--no-daemon" );
    do_noDaemonSpecifiedTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_readCommandLineOptions_nodaemon_short, 
    TestServerConfigReader
)
{
    setupParameters ("-n" );
    do_noDaemonSpecifiedTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_readCommandLineOptions_nodaemon_not_specified, 
    TestServerConfigReader
)
{
    setupParameters ("--help" );
    do_noDaemonNotSpecifiedTest();
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ========================================================================== */

void ServerConfigReader::storeAsString 
(
    const std::string& aName
)
{
    bool isFound = _vm.count(aName);
    assert(isFound);

    if (isFound)
    {
		_vars[aName] = boost::lexical_cast<std::string>(_vm[aName].as<int>());
    }
}

/* ---------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST

BOOST_FIXTURE_TEST_CASE 
(
    Config_ServerConfigReader_storeAsString, 
    TestServerConfigReader
)
{
    setupParameters("--intpar=10");
    po::store(po::parse_command_line(argc, argv, testDesc), _vm);
    po::notify(_vm);
    // Execute test - with existing parameter
    storeAsString ("intpar");
    // Do the checks
    BOOST_CHECK_EQUAL (_vars["intpar"], std::string ("10"));
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ---------------------------------------------------------------------- */

void ServerConfigReader::storeValuesAsStrings ()
{
    storeAsString("Port");
    storeAsString("ThreadNum");
}

/* ========================================================================== */

#ifdef FTS3_COMPILE_WITH_UNITTEST

/** Traits to test _readConfigFile */
struct readConfigFile_TestTraits
{
    static boost::shared_ptr<std::istream> getStream (const std::string&)
    {
        std::stringstream* ss = new std::stringstream;
        assert(ss);
        ss->str("");
        *ss << "intpar=10" << std::endl;
        boost::shared_ptr<std::istream> ret (dynamic_cast<std::istream*>(ss));
        return ret;
    }

    /* ---------------------------------------------------------------------- */
    
    static void processVariables
    (
        ServerConfigReader& reader
    )
    {
        reader.storeAsString("intpar");
    }
};

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Config_ServerConfigReader_readConfigFile, TestServerConfigReader)
{
    _vars["configfile"] = "anyname";
    _readConfigFile<readConfigFile_TestTraits>(testDesc);
    BOOST_CHECK_EQUAL (_vars["intpar"], std::string("10"));
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_CONFIG_NAMESPACE_END

