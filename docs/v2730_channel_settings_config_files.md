
# V2730 Channel settings config files

The individual channel settings config file is set up with 32 rows of parameters, one for each channel of the V2730. Parameters are separated by a comma with no
spaces in between the parameters or after the last parameter in a row. The parameters in the rows are in the following order:

| Parameter Name | Type | Description |
|:---------------|------|-------------|
|Channel number  | int  | Channel number (0-31) |
| DC offset      | float  | Percentage of dynamic range for DC offset (50.0 corresponds to baseline of 0V) |
| Trigger threshold | int | 14-bit signed trigger threshold (relative to baseline) |
| Gain     | int | The gain for each channel in dB (6dB = 2Vpp dynamic range)  |
| Self trigger mode | string | Self trigger propagation for chan n/n+1 pair (only set for even channels). Must be "DISABLED"/"ACQ_ONLY"/"EXTOUT_ONLY"/"ACQ_AND_EXTOUT" (NOT USED)|
| Self trigger logic | string | chan n/n+1 pair self trigger logic (only set for even channels. Must be "AND"/"OR"/"Xn" (for exclusively chan n)/"Xn+1" (for exclusively chan n+1) (NOT USED) |
