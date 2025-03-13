# V2730 config files
The parameters in this file are for global board settings. Individual channel settings can be set in the [individual channel settings configuration file](v2730_channel_settings_config_files.md).

| Parameter Name   | Type      | Description                                                                                      | 
|:-----------------|-----------|--------------------------------------------------------------------------------------------------|
| verbose          | int       | Print confirmation messages (0 or 1)                                                             |
| show_data_rate   | int       | Print data rate every 2 seconds (0 or 1)                                                         |
| store_temps      | int       | Save ADC temps to a text file (0 or 1) (not yet implemented)                                     |
| temp_time        | int       | Time between ADC temp readings in seconds (not yet implemented)                                  |
| VME_bridge       | int       | Digitizer is connected via VME bridge (0 or 1) (NOT USED)                                        |
| address          | uint32_t  | Digitizer VME base address (hex) (only used if VME_bridge=1) (NOT USED)                          |
| LinkNum          | int       | Optical link port on A3818 (NOT USED)                                                            |
| ConetNode        | int       | Digitzer optical link node in daisy chain (NOT USED)                                             |
| path             | string    | Connection path to digitizer in CAEN_FELib                                                       |
| bID              | int       | Board ID                                                                                         |
| use_global       | int       | Use global settings (0 or 1, 0 will use individual channel settings)                             |
| gain             | int       | Gain of dynamic range in dB. (For 2Vpp, gain=6)                                                  |
| RecLen           | uint32_t  | Acquisition window length in ns                                                                  |
| DCOff            | float     | DC offset in percentage of full dynamic range (50% corresponds to 0V baseline)                   |
| use_ETTT         | int       | Use the extended trigger time tag (0 or 1) (NOT USED)                                            |
| ChanEnableMask   | uint32_t  | 32-bit mask of active channels (hex)                                                             |
| PreTrig          | uint32_t  | Time before trigger in ns (rounded to nearest 8 ns)                                              |
| thresh           | int       | Signed trigger threshold in LSB (relative to baseline)                                           |
| polarity         | string    | Trigger on rising edge ("positive") or falling edge ("negative")                                 |
| ChanSelfTrigMask | uint32_t  | 32-bit mask of channels to activate self trigger (hex)                                           |
| ChanSelfTrigEnable| int      | Channel self trigger enable (0 or 1)                                                             |
| TrigInEnable     | int       | External trigger enable (0 or 1)                                                                 |
| SWTrigEnable     | int       | Software trigger enable (0 or 1)                                                                 |
| SWTrigRate       | float     | Software trigger rate in Hz (1kHz limit)                                                         |
| ChanSelfTrigOut  | int       | Propagate channel self trigger to TRGOUT (0 or 1)                                                |
| TrigInOut        | int       | Propagate external trigger to TRGOUT (0 or 1)                                                    |
| SWTrigOut        | int       | Propagate software trigger to TRGOUT (0 or 1)                                                    |
| IOLevel          | string    | Logic level for front panel I/O (Must be "NIM" or "TTL")                                         |
| coincidence_level| int       | For a coincidence_level = m, channel self trigger requires <= m+1 hits to trigger                |
| pulse_width      | int       | Width of self trigger pulse in ns (rounded to nearest 8 ns)                                      |
| chan_set_file    | string    | Path to individual channel settings file                                                         |
| ev_per_file      | int       | Number of events per output file                                                                 |
| ofile            | string    | Output file name or path (timestamp and file number automatically appended to ofile path)        |
