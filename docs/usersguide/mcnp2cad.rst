mcnp2cad
========

UW has a tool, mcnp2cad_, which can import an MCNP model into Coreform Cubit.
This capability is now shipped with Coreform Cubit as of version 2023.11.

mcnp2cad_ builds CAD solids from MCNP cell descriptions, often turning infinite
bodies and planes into finite versions. Please refer to Coreform's
`documentation _coreform_mcnp_import` for usage of this command

Upon completion of the import, a CAD version of the MCNP input is created. If
present, material and cell importance assignments are applied using Cubit group
assignments, as described in _grouping-basics.

..  _mcnp2cad: https://github.com/svalinn/mcnp2cad
..  _coreform_mcnp_import: https://coreform.com/cubit_help/cubithelp.htm#t=geometry%2Fimport%2Fimporting_mcnp_files.htm
