# Chirpbox procedure manager
import sys
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
4) \"python cbmng.py -start [flash_protection] [version_hash] [used_SF] [com_port] [bitmap] [slot_num] [used_tp] \" to start an experiment. The duration of the experiment is defined with -ec and the firmware should be disseminated in advance with -dissem. If flash_protection is 1, daemon protects BANK 1 before switching to BANK 2; if flash_protection is 0, daemon does nothing before switching to BANK 2.\n\
5) \"python cbmng.py -rstatus\" to know the running status of testbed, i.e., whether the testbed is busy or idle\n\
6) \"python cbmng.py -dissem [upgrading_daemon] [ver_hash] [payload_len] [generate_size_in_round] [used_SF] [com_port] [bitmap] [slot_num][dissem_back_sf][dissem_back_slot][used_tp][task_bitmap] \" to disseminate the file, e.g., the firmware. A patch for daemon is generated if upgrading_daemon is 1.\n\
7) \"python cbmng.py -coldata [payload_len] [used_SF] [com_port] [slot_num] [used_tp][task_bitmap] \" to collect the results in the given area of the flash (from [start_addr] to [end_addr]). These addresses are assigned in the methodology file.\n\
8) \"python cbmng.py -connect [SF] [Channel] [Tx_power] [used_SF] [com_port] [slot_num] [topo_payload_len][used_tp] \" to evaluate connectivity of the network with a given SF, a given Channel (KHz), and a given Tx_power (dBm).\n\
9) \"python cbmng.py -colver [used_SF] [com_port] [slot_num][used_tp]\" to obtain the daemon version.\n\
10) \"python cbmng.py -upgrade [filename] [ver_hash] [payload_len] [generate_size_in_round] [used_SF] [com_port] [bitmap] [slot_num][dissem_back_sf][dissem_back_slot][used_tp][task_bitmap]\" to upgrade the daemon. The filename is the updated daemon.bin.")
expconfapp = cbmng_exp_config.myExpConfApproach()
expfirmapp = cbmng_exp_firm.myExpFirmwareApproach()
expmethapp = cbmng_exp_method.myExpMethodApproach()

"""
usage:
python cbmng.py -dissem -daemon_version -packet_length -packet_number_per_round -disseminate_SF -com_port -upgrade_bitmap -disseminate_slot_number -collect_SF -collect_slot_number -tx_power -allnodes_bitmap -upgrade_daemon_or_FUT

python cbmng.py -start -flash_protection -daemon_version -disseminate_SF -com_port -upgrade_bitmap -disseminate_slot_number -tx_power

python cbmng.py -coldata -packet_length -collect_SF -com_port -collect_slot_number -tx_power -allnodes_bitmap -flash_start -flash_end

python cbmng.py -colver -collect_SF -com_port -collect_slot_number -tx_power

python cbmng.py -upgrade -upgrade_bin_name -daemon_version -packet_length -packet_number_per_round -disseminate_SF -com_port -upgrade_bitmap -disseminate_slot_number -collect_SF -collect_slot_number -tx_power -allnodes_bitmap -upgrade_daemon_or_FUT

example:
python cbmng.py -colver 7 com7 80 14
python cbmng.py -dissem 2b10 232 16 7 com7 1fffff 80 7 80 14 1fffff 0
python cbmng.py -start 0 2b10 7 com7 1fffff 100 14
python cbmng.py -coldata 232 7 com7 80 14 1fffff 0807F800 0807FE30
python cbmng.py -upgrade Daemon.bin 2b10 48 4 7 com7 1fffff 100 7 100 14 1fffff 1
python cbmng.py -connect 63 470000 0 7 com7 80 8 14
"""

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
	elif(((argv[1] == "experiment_start") or (argv[1] == "-start")) and (len(argv) == 9)):
	 	if(cbmng_exp_start.check() == True):
	 		cbmng_exp_start.start(argv[5], int(argv[2]), argv[3], int(argv[4]), argv[6], int(argv[7]), int(argv[8]))
	 	# exit(0)
	elif(((argv[1] == "experiment_running_status") or (argv[1] == "-rstatus")) and (len(argv) == 2)):
		if(cbmng_exp_start.is_running() == True):
			print("The testbed is running...")
			exit(1)
		else:
			print("The testbed is idle...")
			exit(0)
	elif(((argv[1] == "connectivity_evaluation") or (argv[1] == "-connect")) and (len(argv) == 10)):
		cbmng_exp_start.connectivity_evaluation(int(argv[2]), int(argv[3]), int(argv[4]), int(argv[5]), argv[6], int(argv[7]), int(argv[8]), int(argv[9]))
		# exit(1)
	elif(((argv[1] == "collect_data") or (argv[1] == "-coldata")) and (len(argv) == 10)):
		if(cbmng_exp_start.is_running() == False):
			cbmng_exp_start.collect_data(argv[4], int(argv[2]), int(argv[3]), int(argv[5]), int(argv[6]), argv[7], argv[8], argv[9])
		# exit(0)
	elif(((argv[1] == "collect_version") or (argv[1] == "-colver")) and (len(argv) == 6)):
		if(cbmng_exp_start.is_running() == False):
			cbmng_exp_start.collect_version(argv[3], int(argv[2]), int(argv[4]), int(argv[5]))
		# exit(0)
	elif(((argv[1] == "disseminate") or (argv[1] == "-dissem")) and (len(argv) == 14)):
		if(cbmng_exp_start.check() == True):
			cbmng_exp_start.disseminate(argv[6], argv[2], int(argv[3]), int(argv[5]), int(argv[4]), argv[7], int(argv[8]), int(argv[9]), int(argv[10]), int(argv[11]), argv[12], int(argv[13]))
		# exit(0)
	# TODO:
	# if bitmap is not full should not use upgrade
	elif(((argv[1] == "upgrade") or (argv[1] == "-upgrade")) and (len(argv) == 15)):
		if(expfirmapp.experiment_firmware(argv[2]) == True):
			expfirmapp.read_configuration()
		cbmng_exp_start.generate_json_for_upgrade()
		if(expconfapp.experiment_configuration("tmp_upgrade.json") == True):
			expconfapp.read_configuration()
		if(expmethapp.experiment_methodology("tmp_upgrade.json") == True):
			expmethapp.read_configuration()
		cbmng_exp_start.disseminate(argv[7], argv[3], int(argv[4]), int(argv[6]), int(argv[5]), argv[8], int(argv[9]), int(argv[10]), int(argv[11]), int(argv[12]), argv[13], int(argv[14]))
		cbmng_exp_start.start(argv[7], 0, argv[3], int(argv[6]), argv[8], int(argv[9]), int(argv[12]))
		exit(0)
	elif(((argv[1] == "help") or (argv[1] == "-h")) and (len(argv) == 2)):
		print_help_text()
		exit(0)
	else:
		print("Please input a valid command")
		exit(1)

if __name__ == "__main__":
    # execute only if run as a script
    main(sys.argv)



