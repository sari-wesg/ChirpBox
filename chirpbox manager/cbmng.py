# Chirpbox procedure manager
import sys
import os
import copy
import cbmng_exp_config
import cbmng_exp_firm
import cbmng_exp_method
import cbmng_exp_start

Chirpbox_procedure_manager_version = 0.0


def get_current_version(a):
    print("Chirpbox: current version is " + str(a))

def print_help_text():
	print("*********************************************")
	print("**************Chirpbox Help******************")
	print("*********************************************")
	print("Chirpbox: Help\n\
One can do as follows to prepare an experiment:\n\
1) Configure an experiment configuration file, including payload length, traffic pattern, interference, and experiment duration\n\
2) Configure a firmware file (.bin) to be tested\n\
3) Configure a methodology file to assign the metrics to be computed\n\
As soon as all the steps mentioned above have been configured properly, then one can start experiments.")
	print("The commands are:\n\
1) \"python cbmng.py -ec [filename]\" to configure experiments\n\
2) \"python cbmng.py -ef [filename]\" to configure firmware\n\
3) \"python cbmng.py -em [filename]\" to configure methodology\n\
4) \"python cbmng.py -start [com_port]\" to start an experiment. The duration of the experiment is defined with -ec and the firmware should be disseminated in advance with -dissem\n\
5) \"python cbmng.py -rstatus\" to know the running status of testbed, i.e., whether the testbed is busy or idle\n\
6) \"python cbmng.py -dissem [com_port]\" to disseminate the file, e.g., the firmware\n\
7) \"python cbmng.py -coldata [com_port]\" to collect the results in the given area of the flash (from [start_addr] to [end_addr]). These addresses are assigned in the methodology file.\n\
8) \"python cbmng.py -connect [SF] [Channel] [Tx_power] [com_port]\" to evaluate connectivity of the network with a given SF, a given Channel (KHz), and a given Tx_power (dBm).\n\
9) \"python cbmng.py -coltopo [using_pos] [com_port]\" to obtain the topology. The using_pos is used to show whether a position layout is used. \n\
10) \"python cbmng.py -assignsnf [com_port]\" to assign a node to work as a sniffer at a given channel (KHz). Sniffers and channels are given in the methodology file.\n")

expconfapp = cbmng_exp_config.myExpConfApproach()
expfirmapp = cbmng_exp_firm.myExpFirmwareApproach()
expmethapp = cbmng_exp_method.myExpMethodApproach()

def main(argv):
	if(((argv[1] == "version") or (argv[1] == "-v")) and (len(argv) == 2)):
		get_current_version(Chirpbox_procedure_manager_version)
	elif(((argv[1] == "experiment_configuration") or (argv[1] == "-ec")) and (len(argv) == 3)):
		if(expconfapp.experiment_configuration(argv[2]) == True):
			expconfapp.read_configuration()
		else:
			exit(0)
	elif(((argv[1] == "experiment_firmware") or (argv[1] == "-ef")) and (len(argv) == 3)):
		if(expfirmapp.experiment_firmware(argv[2]) == True):
			expfirmapp.read_configuration()
		else:
			exit(0)
	elif(((argv[1] == "experiment_methodology") or (argv[1] == "-em")) and (len(argv) == 3)):
		if(expmethapp.experiment_methodology(argv[2]) == True):
			expmethapp.read_configuration()
		else:
	 	 	exit(0)
	elif(((argv[1] == "experiment_start") or (argv[1] == "-start")) and (len(argv) == 3)):
	 	if(cbmng_exp_start.check() == True):
	 		cbmng_exp_start.start(argv[2])
	 	exit(0)
	elif(((argv[1] == "experiment_running_status") or (argv[1] == "-rstatus")) and (len(argv) == 2)):
		if(cbmng_exp_start.is_running() == True):
			print("The testbed is running...")
			exit(1)
		else:
			print("The testbed is idle...")
			exit(0)
	elif(((argv[1] == "connectivity_evaluation") or (argv[1] == "-connect")) and (len(argv) == 6)):
		cbmng_exp_start.connectivity_evaluation(int(argv[2]), int(argv[3]), int(argv[4]), argv[5])
		exit(1)
	elif(((argv[1] == "assign_sniffer") or (argv[1] == "-assignsnf")) and (len(argv) == 3)):
		if(cbmng_exp_start.check() == True):
			cbmng_exp_start.assign_sniffer(argv[2])
		exit(0)
	elif(((argv[1] == "collect_data") or (argv[1] == "-coldata")) and (len(argv) == 3)):
		if((cbmng_exp_start.check_finished() == True) and (cbmng_exp_start.is_running() == False)):
			cbmng_exp_start.collect_data(argv[2])
		exit(0)
	elif(((argv[1] == "collect_topology") or (argv[1] == "-coltopo")) and (len(argv) == 4)):
		if((cbmng_exp_start.check_finished() == True) and (cbmng_exp_start.is_running() == False)):
			cbmng_exp_start.collect_topology(argv[3], argv[2])
		exit(0)
	elif(((argv[1] == "disseminate") or (argv[1] == "-dissem")) and (len(argv) == 3)):
		if(cbmng_exp_start.check() == True):
			cbmng_exp_start.disseminate(argv[2])
		exit(0)		
	elif(((argv[1] == "help") or (argv[1] == "-h")) and (len(argv) == 2)):
		print_help_text()
		exit(0)
	else:
		print("Please input a valid command")
    
if __name__ == "__main__":
    # execute only if run as a script
    main(sys.argv)



