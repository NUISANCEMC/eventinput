# Weight Tuning Tests
import pyNUISANCE as pn
pn.log.set_level(pn.log.level.trace)

evs = pn.EventSource("../../runs/NEUT-D2-ANL_77-numu/NEUT.numu.numu_flux.ANL_1977_2horn_rescan.20217.evts.root")
if not evs: print("Error: failed to open input file")

weightcalc = pn.WeightCalcFactory().make(evs, {"neut_cardname": "./neut.card"})

print("FINISHED RW")

weightcalc.set_parameters({"MaCCQE":1.0})
print("SET PARS")