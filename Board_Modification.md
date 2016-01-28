This page describes board modifications that you can try if you're not afraid of cutting traces and potentially damaging your board.

**Controlling Channel Readout**

We've tied the ADC SELECT line to the clock line as suggested by the AD9201 datasheet.  That way, both channels are sampled on the rising clock edge, and the data for the first channel appears about 20ns later.  Then data for the second channel appears about 20ns after the falling clock edge.

If you only care about one channel at a time, or perhaps the 20ns latency after each clock edge is giving you trouble (a full cycle at 20MHz is only 50ns!), a simple board modification will let you control the select line yourself.

![Channel Readout Modification Diagram](channel_readout_modification.jpg?raw=true "Modifying the board for channel readout control")

We provided two vias near pin 13 on the ADC, so that you could drill out the small via (carefully drilling only the diameter of the via to avoid accidentally cutting the nearby clock trace), then control the SELECT line from the big via.  (It's big enough to hold a 0.100" header pin.)

**Differential Input support**

Both channels on the AD9201 support differential input, but our design is for single-ended input.  We've made it easier to get access to the "INB-I" and "INB-Q" inputs for those who need that, but we haven't tried this out ourselves, so proceed with caution.  This mod is also relevant if you need to do something different with VREF or VREFSENSE.

![Differential Input Modification Diagram](differential_input_modification.jpg?raw=true "Modifying the board for differential input")

To get access to INB-I and INB-Q, remove R1, R22, C6 and C30, and cut the trace between R11 and R1.  Then the vias above and below the cut will give you direct access to INB-Q and INB-I.  The via below the cut is also routed to the 4 header pins labeled "VREF" in the black header, so those would also now be connected to INB-I.  The via to the right of R1 can then be used instead to access VREF.

If you need to modify VREFSENSE, your best bet may be to remove C17 and C21, the analog VCC bypass caps, then drill out the via that grounds these two caps and the VREFSENSE pin.
