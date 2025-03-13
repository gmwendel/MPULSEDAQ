# V1730 config files
The parameters in this file are for global board settings. Individual channel settings can be set in the [individual channel settings configuration file](v1730_channel_settings_config_files.md).

| Parameter Name   | Type      | Description                                                                                      | 
|:-----------------|-----------|--------------------------------------------------------------------------------------------------|
| verbose          | int       | Print confirmation messages (0 or 1)                                                             |
| show_data_rate   | int       | Print data rate every 2 seconds (0 or 1)                                                         |
| VME_bridge       | int       | Digitizer is connected via VME bridge (0 or 1)                                                   |
| address          | uint32_t  | Digitizer VME base address (hex) (only used if VME_bridge=1)                                     |
| LinkNum          | int       | Optical link port on A3818                                                                       |
| ConetNode        | int       | Digitzer optical link node in daisy chain                                                        |
| bID              | int       | Board ID                                                                                         |
| use_global       | int       | Use global settings (0 or 1, 0 will use individual channel settings)                             |
| DynRange         | float     | Input dynamic range (must be 0.5 or 2.0)                                                         |
| RecLen           | uint32_t  | Acquisition window length in ns (will round to nearest 40 ns)                                    |
| DCOff            | uint32_t  | DC offset in percentage of full dynamic range (50% corresponds to 0V baseline                    |
| use_ETTT         | int       | Use the extended trigger time tag (0 or 1)                                                       |
| ChanEnableMask   | uint32_t  | 16-bit mask of active channels (hex)                                                             |
| PostTrig         | uint32_t  | Time after trigger (ns)                                                                          |
| thresh           | uint32_t  | Trigger threshold in LSB (not relative to baseline)                                              |
| polarity         | string    | Trigger on rising edge ("positive") or falling edge ("negative")                                 |
| ChanSelfTrigMask | uint32_t  | 16-bit mask of channels to activate self trigger (hex)                                           |
| ChanSelfTrigMode | string    | Channel self trigger propagation. Must be "DISABLED"/"ACQ_ONLY"/"EXTOUT_ONLY"/"ACQ_AND_EXTOUT"   |
| TrigInMode       | string    | External trigger input propagation. Must be "DISABLED"/"ACQ_ONLY"/"EXTOUT_ONLY"/"ACQ_AND_EXTOUT" |
| SWTrigMode       | string    | SW trigger propagation. Must be "DISABLED"/"ACQ_ONLY"/"EXTOUT_ONLY"/"ACQ_AND_EXTOUT"             |
| IOLevel          | string    | Logic level for front panel I/O (Must be "NIM" or "TTL")                                         |
| coincidence_level| int       | For a coincidence_level = m, channel self trigger requires <= m+1 hits to trigger                |
| coincidence_window| int      | Time window for coincidence trigger in 8 ns ticks                                                |
| chan_set_file    | string    | Path to individual channel settings file                                                         |
| ev_per_file      | int       | Number of events per output file                                                                 |
| ofile            | string    | Output file name or path (timestamp and file number automatically appended to ofile path)        |
