import os
import sys
import pulsar as psr

def run(mm):
    tester=psr.PyTester("Testing the Pulsar-ForceManII API")
    wfn=psr.Wavefunction()
    wfn.system=psr.make_system("""
    0 1
    O     0.0 0.0 0.0
    H     1.0 0.0 0.0
    H     0.0 1.0 0.0
    """)
    atom_types=[2001,2002,2002]
    ff="AMBER99"
    model="HARMONICOSCILLATOR"
    coord="BOND"
    mod_key="H.O."
    mm.load_module("fmanii","FFTerm",mod_key)
    mm.load_module("fmanii","FFCharges","Chgs")
    mm.change_option(mod_key,"FORCE_FIELD",ff)
    mm.change_option(mod_key,"ATOM_TYPES",atom_types)
    mm.change_option("Chgs","FORCE_FIELD",ff)
    mm.change_option("Chgs","ATOM_TYPES",atom_types)
    mm.change_option(mod_key,"MODEL_NAME",model)
    mm.change_option(mod_key,"COORD_NAME",coord)
    my_mod=mm.get_module(mod_key,0)
    newwfn,deriv=my_mod.deriv(0,wfn)
    tester.test_double("H.O. Bond stretch",deriv[0],0.0032286707269852354)
    my_mod=mm.get_module("Chgs",0)
    chgs=my_mod.calculate(0,wfn)
    corr_chgs=[-0.834, 0.417, 0.417]
    for q1,q2 in zip(chgs,corr_chgs):
        tester.test_double("Charge",q1,q2)
    tester.print_results()
    return tester.nfailed()

def run_test():
    with psr.ModuleAdministrator() as mm:
        return run(mm)

