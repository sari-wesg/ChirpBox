import cbmng_common
import json
import os

class myExpMethodApproach(cbmng_common.ExpMethodApproach):
	num_generated_packets = False
	num_received_packets = False
	e2e_latency = False
	tx_energy = False
	rx_energy = False
	start_address = ''
	end_address = ''
	experiment_run_bitmap = ''
	experiment_run_time = 0

	def read_configuration(self):
		with open(cbmng_common.ExpMethodApproach.methodology_file,'r') as load_f:
			load_dict = json.load(load_f)
			# The number of generated packets:
			myExpMethodApproach.num_generated_packets = load_dict['num_generated_packets']
			print("num_generated_packets: " + str(myExpMethodApproach.num_generated_packets))
			# The number of received packets:
			myExpMethodApproach.num_received_packets = load_dict['num_received_packets']
			print("num_received_packets: " + str(myExpMethodApproach.num_received_packets))
			# End-to-end latench:
			myExpMethodApproach.e2e_latency = load_dict['e2e_latency']
			print("e2e_latency: " + str(myExpMethodApproach.e2e_latency))
			# Tx time:
			myExpMethodApproach.tx_energy = load_dict['tx_energy']
			print("tx_energy: " + str(myExpMethodApproach.tx_energy))
			# Rx time:
			myExpMethodApproach.rx_energy = load_dict['rx_energy']
			print("rx_energy: " + str(myExpMethodApproach.rx_energy))
			# Start_address:
			myExpMethodApproach.start_address = load_dict['start_address']
			print("start_address: " + str(myExpMethodApproach.start_address))
			# End_address:
			myExpMethodApproach.end_address = load_dict['end_address']
			print("end_address: " + str(myExpMethodApproach.end_address))
			# Experiment_run_time:
			myExpMethodApproach.experiment_run_bitmap = load_dict['experiment_run_bitmap']
			print("experiment_run_bitmap: " + str(myExpMethodApproach.experiment_run_bitmap))
			# Experiment_run_time:
			myExpMethodApproach.experiment_run_time = int(load_dict['experiment_run_time'])
			print("experiment_run_time: " + str(myExpMethodApproach.experiment_run_time))
		with open(os.path.join(os.path.dirname(__file__), "tmp_exp_meth.json"),"w") as dump_f:
			json.dump(load_dict,dump_f)
		# TODO: Add some codes here