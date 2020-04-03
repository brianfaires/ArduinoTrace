// ArduinoTrace - github.com/bblanchon/ArduinoTrace
// Copyright Benoit Blanchon 2018-2019
// MIT License
//
// A simple tracing macro to debug you program.
//
// Recipe to find where the code crashes:
//  1. sprinkle your code with TRACE()
//  2. run the program
//  3. view all traces in the Serial monitor
//
// Each trace includes the:
//  * the filename
//  * the line number
//  * the current function
//  * the template parameters (if any)

#pragma once

#include <Arduino.h>

#ifndef ARDUINOTRACE_ENABLE
#define ARDUINOTRACE_ENABLE 1
#endif

#if ARDUINOTRACE_ENABLE == 1

#ifndef ARDUINOTRACE_SERIAL
#define ARDUINOTRACE_SERIAL Serial
#endif

#ifndef ARDUINOTRACE_ENABLE_PROGMEM
#ifdef PROGMEM
#define ARDUINOTRACE_ENABLE_PROGMEM 1
#else
#define ARDUINOTRACE_ENABLE_PROGMEM 0
#endif
#endif

#ifndef ARDUINOTRACE_ENABLE_FULLPATH
#define ARDUINOTRACE_ENABLE_FULLPATH 0
#else
#define ARDUINOTRACE_ENABLE_FULLPATH 1
#endif

#if ARDUINOTRACE_ENABLE_PROGMEM
#define ARDUINOTRACE_FLASHIFY(X) F(X)
#else
#define ARDUINOTRACE_FLASHIFY(X) X
#endif

namespace ArduinoTrace {
constexpr size_t strlen(const char *str) {
  return str[0] ? strlen(str + 1) + 1 : 0;
}

template <char... chars>
struct string {
#if ARDUINOTRACE_ENABLE_PROGMEM
  const __FlashStringHelper *data() {
    static const char buffer[] PROGMEM = {chars...};
    return reinterpret_cast<const __FlashStringHelper *>(buffer);
  }
#else
  const char *data() {
    static const char buffer[] = {chars...};
    return buffer;
  }
#endif
};

template <typename TSourceString, size_t remainingLength,
          char... collectedChars>
struct string_maker {
  using result =
      typename string_maker<TSourceString, remainingLength - 1,
                            TSourceString::data()[remainingLength - 1],
                            collectedChars...>::result;
};

#if ARDUINOTRACE_ENABLE_FULLPATH == 0
template <typename TSourceString, size_t remainingLength,
          char... collectedChars>
struct string_maker<TSourceString, remainingLength, '/', collectedChars...> {
  using result = string<collectedChars..., '\0'>;
};

template <typename TSourceString, size_t remainingLength,
          char... collectedChars>
struct string_maker<TSourceString, remainingLength, '\\', collectedChars...> {
  using result = string<collectedChars..., '\0'>;
};
#endif

template <typename TSourceString, char... collectedChars>
struct string_maker<TSourceString, 0, collectedChars...> {
  using result = string<collectedChars..., '\0'>;
};

template <typename TStringSource>
using make_string =
    typename string_maker<TStringSource, strlen(TStringSource::data())>::result;

struct Initializer {
  template <typename TSerial>
  Initializer(TSerial &serial, int bauds) {
    serial.begin(bauds);
    while (!serial) continue;
  }
};

template <typename TFile>
struct Printer {
  template <typename TSerial, typename TLine, typename TFunction, typename TValue, typename TValue2, typename TPrefix>
  Printer(TSerial &serial, const TLine &line, const TFunction &function, const TValue &content, const TValue2 &content2, const TPrefix &prefix) {
	serial.print(prefix);
    serial.print(make_string<TFile>{}.data());
    serial.print(line);
    serial.print(": ");
	if(content[0]) {
		serial.print(function);
		serial.print(": ");
	    serial.print(content);
	    serial.println(content2);
	}
	else {
		serial.println(function);
	}

	serial.flush();
  }
};
}  // namespace ArduinoTrace


#define ARDUINOTRACE_STRINGIFY(X) #X
#define ARDUINOTRACE_CONCAT(X, Y) X##Y

#define ARDUINOTRACE_PRINT(prefix, file, line, function, content, content2)               \
  {                                                                           \
    struct __file {                                                         \
      constexpr static char const *data() { return file; }                  \
    }; 																		\
    ArduinoTrace::Printer<__file> __tracer(ARDUINOTRACE_SERIAL, line, function, content, content2, prefix); \
  }


// Write traces to the Serial port
//
#define TRACE() 	   ARDUINOTRACE_PRINT("    "			, __FILE__ ":", __LINE__, __PRETTY_FUNCTION__,  "", "")
#define DEBUG(message) ARDUINOTRACE_PRINT("    "			, __FILE__ ":", __LINE__, __PRETTY_FUNCTION__, String() + message, "")
#define DUMP(variable) ARDUINOTRACE_PRINT("    "			, __FILE__ ":", __LINE__, __PRETTY_FUNCTION__, #variable " = ", variable)
#define THROW(message) ARDUINOTRACE_PRINT("******* ERROR: "	, __FILE__ ":", __LINE__, __PRETTY_FUNCTION__, String() + message, "")
#define THROW_DUMP(message, variable) THROW(message) DUMP(variable)
#define PRINT(message) Serial.print(String() + message)
#define PRINTLN(message) Serial.println(String() + message)


// Initializes the Serial port
//
// Use this macro only if you want to call TRACE() at global scope,
// in other cases, call Serial.begin() in your setup() function, as usual.
#define ARDUINOTRACE_INITIALIZE(id, bauds) ArduinoTrace::Initializer ARDUINOTRACE_CONCAT(__initializer, id)(ARDUINOTRACE_SERIAL, bauds);
#define ARDUINOTRACE_INIT(bauds) ARDUINOTRACE_INITIALIZE(__COUNTER__, bauds);

#else  // ie ARDUINOTRACE_ENABLE == 0
#define ARDUINOTRACE_INIT(bauds)
#define TRACE()
#define DUMP(variable)
#define THROW(message)
#define THROW(message, variable)
#define DEBUG(message)
#endif