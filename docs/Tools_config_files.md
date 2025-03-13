# Tools Config Files

Tools config files are the list of tools to be added to the Toolchain that will be run (in order) by ToolDAQ. 
The format is `<Unique Tool Instance Name> <Tool Name> <path to config file to pass to tool>`. 
For example, to add 2 ReadV1730 tools with different config files to the Toolchain, then a ReadV2730 tool, the ToolsConfig file would look like:
```
ReadV17301 ReadV1730 configfiles/v1730/config_b1
ReadV17302 ReadV1730 configfiles/v1730/config_b2
ReadV27301 ReadV2730 configfiles/v2730/config_b3
```
