\mainpage Crystal Test Framework LUA interface

# Introduction
The Crystal Test Framework is written to automate  tests of hardware and embedded systems. It mainly consists of three parts:

* Communication
> Handles communication over COM ports or virtual COM ports. Different protocols are/will be implemented. The most used protocols are SCPI/USBTMC and [RPC ]([https://github.com/Crystal-Photonics/RPC-Generator](https://github.com/Crystal-Photonics/RPC-Generator) ). SCPI is a common protocol to talk with laboratory devices such as multimeters, oscilloscopes, power supplies, frequency generators etc. The RPC protocol is a generic protocol to talk with embedded systems.

* Script-Engine
> Loads and executes *.lua scripts which define the measurement and test procedure.

* Data- and Report-Engine
> Manages the desired measurement results together with its tolerances and actual values which can be printed into a pdf report.

## Script-Engine
For using this framework it is essential to write scripts which define the test procedure and run within the Crystal Test Framework. The language is [Lua](https://www.lua.org/pil/contents.html). There are built-in functions provided by the Crystal Test Framework to simplify common Test Tasks. Following links/chapters will be useful to understand the interface of the script engine:

* \ref ui "User interaction"

* \ref convenience "Built-in convenience functions"

* Data- and report engine 
