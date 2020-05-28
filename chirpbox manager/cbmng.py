# Chirpbox procedure manager
import sys
import os
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
1) Configure an experiment configuration file, including payload length, traffic pattern, interference, and experiment duration;\n\
2) Configure a firmware file (.bin) to be tested;\n\
3) Configure a methodology file to assign the metrics to be computed;\n\
As soon as all the steps mentioned above have been configured properly, then one can start experiments.")
	print("The commands are:\n\
1) \"python cbmng.py -ec [filename]\" to configure experiments;\n\
2) \"python cbmng.py -ef [filename]\" to configure firmware;\n\
3) \"python cbmng.py -em [filename]\" to configure methodology;\n\
4) \"python cbmng.py -start\" to start an experiment. The duration of the experiment is defined with -ec and the firmware should be disseminated in advance with -dissem;\n\
5) \"python cbmng.py -rstatus\" to know the running status of testbed, i.e., whether the testbed is busy or idle\n\
6) \"python cbmng.py -dissem [filename]\" to disseminate the firmware [filename]\n\
7) \"python cbmng.py -coldata\" to collect the results\n\
8) \"python cbmng.py -gettopo [SF] [Channel]\" to evaluate connectivity of the network with a given SF and Channel (MHz)\n\
9) \"python cbmng.py -coltopo\" to obtain the topology\n\
10) \"python cbmng.py -settosnf\" to assign nodes to work as sniffers\n")

expconfapp = cbmng_exp_config.myExpConfApproach()
expfirmapp = cbmng_exp_firm.myExpFirmwareApproach()
expmethapp = cbmng_exp_method.myExpMethodApproach()

def main(argv):
	if(((argv[1] == "version") or (argv[1] == "-v")) and (len(argv) == 2)):
		get_current_version(Chirpbox_procedure_manager_version)
	if(((argv[1] == "experiment_configuration") or (argv[1] == "-ec")) and (len(argv) == 3)):
		if(expconfapp.experiment_configuration(argv[2]) == True):
			expconfapp.read_configuration()
		else:
			exit(0)
	if(((argv[1] == "experiment_firmware") or (argv[1] == "-ef")) and (len(argv) == 3)):
		if(expfirmapp.experiment_firmware(argv[2]) == True):
			expfirmapp.read_configuration()
		else:
			exit(0)
	if(((argv[1] == "experiment_methodology") or (argv[1] == "-em")) and (len(argv) == 3)):
		if(expmethapp.experiment_methodology(argv[2]) == True):
			expmethapp.read_configuration()
		else:
	 	 	exit(0)
	if(((argv[1] == "experiment_start") or (argv[1] == "-start")) and (len(argv) == 2)):
	 	if(cbmng_exp_start.check() == True):
	 		cbmng_exp_start.start();
	 	exit(0)
	if(((argv[1] == "experiment_running_status") or (argv[1] == "-rstatus")) and (len(argv) == 2)):
		if(cbmng_exp_start.is_running() == True):
			print("The testbed is running...")
			exit(1)
		else:
			print("The testbed is idle...")
			exit(0)
	if(((argv[1] == "help") or (argv[1] == "-h")) and (len(argv) == 2)):
		print_help_text()
		exit(0)

	#print("esf")


	#expconfapp.experiment_duration
	

	# print(len(argv))
    # print(argv[1])
    # print(argv[2])
    # print(argv[3])
    
if __name__ == "__main__":
    # execute only if run as a script
    main(sys.argv)



