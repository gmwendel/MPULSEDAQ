
# Channel settings config files

The individual channel settings config file is set up with 16 rows of parameters, one for each channel of the V1730S. Parameters are separated by a comma with no
spaces in between the parameters or after the last parameter in a row. The parameters in the rows are in the following order:

| Parameter Name | Type | Description |
|:---------------|------|-------------|
|Channel number  | int  | Channel number (0-15) |
| DC offset      | float  | Percentage of dynamic range for DC offset (50.0 corresponds to baseline of 0V) |
| Trigger threshold | int | 14-bit trigger threshold (not relative to baseline) |
| Dynamic range     | float | Sets the input dynamic range in Vpp. Must be 0.5 or 2.0  |
| Self trigger mode | string | Self trigger propagation for chan n/n+1 pair (only set for even channels). Must be "DISABLED"/"ACQ_ONLY"/"EXTOUT_ONLY"/"ACQ_AND_EXTOUT"|
| Self trigger logic | string | chan n/n+1 pair self trigger logic (only set for even channels. Must be "AND"/"OR"/"Xn" (for exclusively chan n)/"Xn+1" (for exclusively chan n+1) |
