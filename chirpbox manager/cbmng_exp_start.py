import cbmng_exp_config
import cbmng_exp_firm
import cbmng_exp_method
import cbmng_common
import datetime
import time
import json
import transfer_to_initiator.myserial.serial_send
import statistics_process.topo_parser

import serial
import os

exp_conf = "tmp_exp_conf.json"
firmware = "tmp_exp_firm.bin"
firmware_burned = "tmp_exp_firm_burned.bin"
firmware_daemon_burned = "daemon_firm_burned.bin"
exp_meth = "tmp_exp_meth.json"
running_status = "tmp_exp_running.json"


expconfapp = cbmng_exp_config.myExpConfApproach()
expfirmapp = cbmng_exp_firm.myExpFirmwareApproach()
expmethapp = cbmng_exp_method.myExpMethodApproach()

EXPERIMENT_START = 0
EXPERIMENT_DISSEMINATE = 1
EXPERIMENT_COLDATA = 2
EXPERIMENT_CONNECT = 3
EXPERIMENT_COLTOPO = 4
EXPERIMENT_ASSIGNSNF = 5


def check():
	try:
		f = open(exp_conf, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment completion with \"-ec\"")
		return False
	except PermissionError:
		print("Please complete experiment completion with \"-ec\"")
		return False

	try:
		f = open(firmware, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment firmware with \"-ef\"")
		return False
	except PermissionError:
		print("Please complete experiment firmware with \"-ef\"")
		return False

	try:
		f = open(exp_meth, mode = 'r')
		f.close()
	except FileNotFoundError:
		print("Please complete experiment methodology with \"-em\"")
		return False
	except PermissionError:
		print("Please complete experiment methodology with \"-em\"")
		return False

	return True

def check_finished():
	try:
		f = open(running_status, mode = 'r')
		f.close()
	except FileNotFoundError:
		return False
	return True

def generate_json_for_upgrade():
	upgrade_dict = {
		"experiment_name": "Upgrade_daemon",
		"experiment_description": "Upgrade_daemon",
		"payload_length": 0, 
		"experiment_duration": 30,
		"num_generated_packets": "False",
		"num_received_packets": "False",
		"e2e_latency": "False",
		"tx_energy": "False",
		"rx_energy": "False",
		"sniffer_and_channels": [],
		"start_address": "00000000",
		"end_address": "00000000"
	}
	with open("tmp.json", "w") as f:
		json.dump(upgrade_dict, f)


def start(com_port, flash_protection):
	if(expconfapp.experiment_configuration(exp_conf) == True):
		expconfapp.read_configuration()
		time_now = datetime.datetime.now()
		start_time_t = time_now + datetime.timedelta(seconds = 50)
		start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
		end_time_t = start_time_t + datetime.timedelta(seconds = expconfapp.experiment_duration)
		end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
		exp_no = cbmng_common.tid_maker()
		exp_name = expconfapp.experiment_name
		print("Experiment #" + exp_no + " (" + exp_name + ") is going to start at " + start_time + ", and stop at " + end_time)
		running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': expconfapp.experiment_duration}
		with open(running_status, "w") as f:
			json.dump(running_dict, f)
	else:
		return False

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_START

	timeout_cnt = 0

	ser.write(str(task_index).encode()) # send commands

	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
			 	# if (line == "Input initiator task:"):
			 	#  	time.sleep(2)
			 	#  	print('Input task_index')
				#   ser.write(str(task_index).encode()) # send commands
			 	if (line == "Waiting for parameter(s)..."):
			 		if(flash_protection == 1):
			 			para = start_time_t.strftime("%Y,%m,%d,%H,%M,%S") + "," + end_time_t.strftime("%Y,%m,%d,%H,%M,%S") + ",1"
			 		else:
			 			para = start_time_t.strftime("%Y,%m,%d,%H,%M,%S") + "," + end_time_t.strftime("%Y,%m,%d,%H,%M,%S") + ",0"
			 		print(para)
			 		ser.write(str(para).encode()) # send commands
			 		timeout_cnt = 0
			 		break
			if(timeout_cnt > 1000):
				break
		except:
			pass
	if(timeout_cnt > 1000):
		print("Timeout...")
		return False

	print("Done!")
	return True

def is_running():
	try:
		f = open(running_status, mode = 'r')
		f.close()
	except FileNotFoundError:
		return False

	with open(running_status, 'r') as load_f:
		running_dict = json.load(load_f)
		start_time_t = datetime.datetime.strptime(running_dict['start_time'], '%Y-%m-%d %H:%M:%S')
		end_time_t = datetime.datetime.strptime(running_dict['end_time'], '%Y-%m-%d %H:%M:%S')
		time_now = datetime.datetime.now()
		if (time_now >= start_time_t) and (time_now <= end_time_t):
			return True
		else:
			return False

def connectivity_evaluation(sf, channel, tx_power, com_port):
	time_now = datetime.datetime.now()
	start_time_t = time_now + datetime.timedelta(minutes = 2)
	start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
	end_time_t = start_time_t + datetime.timedelta(minutes = 10)
	end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
	exp_no = cbmng_common.tid_maker()
	exp_name = "Chirpbox_connectivity_sf" + str(sf) + "ch" + str(channel) + "tp" + str(tx_power)
	print("Connectivity evaluation (SF " + str(sf) + ", Channel " + str(channel) + " MHz, " + str(tx_power) + " dBm) is going to start at " + start_time + ", and stop at " + end_time)
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)
	
	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_CONNECT

	timeout_cnt = 0

	ser.write(str(task_index).encode()) # send commands
	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
			 	# if (line == "Input initiator task:"):
			 	#  	time.sleep(2)
			 	#  	print('Input task_index')
				#   ser.write(str(task_index).encode()) # send commands
			 	if (line == "Waiting for parameter(s)..."):
			 		if(tx_power >= 0):
			 			para = '{0:02}'.format(int(sf)) + ',{0:06}'.format(int(channel)) + ',+{0:02}'.format(int(tx_power))
			 		if(tx_power < 0):
			 			para = '{0:02}'.format(int(sf)) + ',{0:06}'.format(int(channel)) + ',-{0:02}'.format(int(tx_power) * (-1))
			 		print(para)
			 		ser.write(str(para).encode()) # send commands
			 		timeout_cnt = 0
			 		break
			if(timeout_cnt > 1000):
				break
		except:
			pass
	if(timeout_cnt > 1000):
		print("Timeout...")
		return False

	if(waiting_for_the_execution_timeout(ser, 800) == False): # timeout: 800 seconds
		return False

	time_now1 = datetime.datetime.now()
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': time_now1.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)
	print("Done!")
	return True

def assign_sniffer(com_port):
	if(expmethapp.experiment_methodology(exp_meth) == True):
		expmethapp.read_configuration()
		time_now = datetime.datetime.now()
		pairs = expmethapp.sniffer_and_channels
		num = len(pairs) / 2
		print("Sniffers and channels: " + str(pairs))
	else:
		return False
	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_ASSIGNSNF

	timeout_cnt = 0
	sniffer_cnt = 0

	ser.write(str(task_index).encode()) # send commands
	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
			 	# if (line == "Input initiator task:"):
			 	#  	time.sleep(2)
			 	#  	print('Input task_index')
				#   ser.write(str(task_index).encode()) # send commands
			 	if (line == "Waiting for parameter(s)..."):
			 	 	if(num == 1):
			 	 		print("There is one sniffer.")
			 	 	if(num > 1):
			 	 		print("There are " + str(int(num)) + " sniffers.")
			 	 	ser.write(str(num).encode()) # send commands
			 	 	timeout_cnt = 0			 	
			 	if (line == "Sniffer config..."):
			 		para = '{0:03}'.format(int(pairs[sniffer_cnt])) + ',{0:06}'.format(int(pairs[sniffer_cnt + 1]))
			 		print(para)
			 		ser.write(str(para).encode()) # send commands
			 		sniffer_cnt = sniffer_cnt + 2
			 		timeout_cnt = 0
			 		if(sniffer_cnt >= len(pairs)):
			 			break
			if(timeout_cnt > 1000):
				break
		except:
			pass
	if(timeout_cnt > 1000):
		print("Timeout...")
		return False

	if(waiting_for_the_execution_timeout(ser, 10) == False): # timeout: 10 seconds
		return False

	print("Done!")
	return True

def collect_data(com_port):
	if(expmethapp.experiment_methodology(exp_meth) == True):
		expmethapp.read_configuration()
		start_addr = expmethapp.start_address
		end_addr = expmethapp.end_address
		print("start address: " + str(start_addr))
		print("end address: " + str(end_addr))
	else:
		return False

	with open(running_status,'r') as load_f:
		load_dict = json.load(load_f)
		filename = load_dict['exp_name'] +"(" + load_dict['exp_number'] + ").txt"
	print("Collecting ...")

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_COLDATA

	timeout_cnt = 0
	sniffer_cnt = 0

	ser.write(str(task_index).encode()) # send commands

	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
			 	# print(line)
			 	# if (line == "Input initiator task:"):
			 	#  	time.sleep(2)
			 	#  	print('Input task_index')
			 	# 	ser.write(str(task_index).encode()) # send commands
			 	#	timeout_cnt = 0
			 	if (line == "Waiting for parameter(s)..."):
			 	 	para = "%08X" % int(start_addr, 16) + "," + "%08X" % int(end_addr, 16)
			 	 	print(para)
			 	 	ser.write(str(para).encode()) # send commands
			 	 	timeout_cnt = 0
			 	 	break
			if(timeout_cnt > 1000):
				break
		except:
			pass
	if(timeout_cnt > 1000):
		print("Timeout...")
		return False

	with open(filename, 'w+') as f:
		start_read = 0
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
					if (line == "output from initiator (topology):"):
						start_read = 1
					if (line == "Task list:"):
						timeout_cnt = 0
						break
					if(start_read == 1):
						f.write(line + "\r")
				if(timeout_cnt > 60000):
					break
			except:
				pass
	if(timeout_cnt > 60000):
		print("Timeout...")
		return False

	print("Results of " + filename + " have been collected!" )
	return True	

def collect_topology(com_port, using_pos):
	with open(running_status,'r') as load_f:
		load_dict = json.load(load_f)
		filename = load_dict['exp_name'] +"(" + load_dict['exp_number'] + ").txt"
	print("Collecting ...")
	
	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_COLTOPO

	timeout_cnt = 0
	sniffer_cnt = 0

	ser.write(str(task_index).encode()) # send commands
	with open(filename, 'w+') as f:
		start_read = 0
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
				 	if (line == "output from initiator (topology):"):
				 		start_read = 1
	 			 	#  	time.sleep(2)
	 			 	#  	print('Input task_index')
	 				#   ser.write(str(task_index).encode()) # send commands
	 				if (line == "Task list:"):
	 					timeout_cnt = 0
	 					break
	 				if(start_read == 1):
	 					f.write(line + "\r")
				if(timeout_cnt > 60000):
	 				break
			except:
	 			pass
	if(timeout_cnt > 60000):
	 	print("Timeout...")
	 	return False

	print("Results of " + filename + " have been collected!" )
	results = statistics_process.topo_parser.topo_parser(filename, using_pos)
	# max_hop, mean_degree_array, std_dev_degree_array, min_dev_degree_array, max_dev_degree_array
	print("Max_hop: " + str(results[0]))
	print("Mean_degree: " + str(results[1]))
	print("Std_dev_degree: " + str(results[2]))
	print("Min_degree: " + str(results[3]))
	print("Max_degree: " + str(results[4]))
	print("Symmetry: " + str(results[5]))
	return True	

def disseminate(com_port, daemon_patch):

	BANK2_SIZE = 512 * 1024

	try:
		if(daemon_patch == 1):
			f = open(firmware_daemon_burned, mode = 'r')
		if(daemon_patch == 0):
			f = open(firmware_burned, mode = 'r')
		f.close()
		firmware_burned_existing = 1
	except FileNotFoundError:
		firmware_burned_existing = 0
	except PermissionError:
		firmware_burned_existing = 0

	if(firmware_burned_existing == 1):
		if(daemon_patch == 1):
			jdiff = ".\JojoDiff\win32\jdiff.exe " + firmware_daemon_burned + " " + firmware + " patch.bin"	
		if(daemon_patch == 0):
			jdiff = ".\JojoDiff\win32\jdiff.exe " + firmware_burned + " " + firmware + " patch.bin"
		print(jdiff)
		r_v = os.system(jdiff) 
		print (r_v)
		print ("Patch size: " + str(cbmng_common.get_FileSize('patch.bin')))
		print ("The updated firmware size: " + str(cbmng_common.get_FileSize(firmware)))
		if(daemon_patch == 1):	
			print ("The burned daemon firmware size: " + str(cbmng_common.get_FileSize(firmware_daemon_burned)))
		if(daemon_patch == 0):
			print ("The burned firmware size: " + str(cbmng_common.get_FileSize(firmware_burned)))		
		if(daemon_patch == 1):
			if((cbmng_common.get_FileSize('patch.bin') < cbmng_common.get_FileSize(firmware)) and (cbmng_common.get_FileSize(firmware) + cbmng_common.get_FileSize('patch.bin') < BANK2_SIZE - 4096) and (cbmng_common.get_FileSize(firmware_daemon_burned) + cbmng_common.get_FileSize('patch.bin') < BANK2_SIZE - 4096)):
				using_patch = 1
				print("disseminate the patch...")
			else:
				using_patch = 0
				print("disseminate the updated firmware...")
		if(daemon_patch == 0):
			if((cbmng_common.get_FileSize('patch.bin') < cbmng_common.get_FileSize(firmware)) and (cbmng_common.get_FileSize(firmware) + cbmng_common.get_FileSize('patch.bin') < BANK2_SIZE - 4096) and (cbmng_common.get_FileSize(firmware_burned) + cbmng_common.get_FileSize('patch.bin') < BANK2_SIZE - 4096)):
				using_patch = 1
				print("disseminate the patch...")
			else:
				using_patch = 0
				print("disseminate the updated firmware...")		
	else:
		using_patch = 0
		print("disseminate the updated firmware...")

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_DISSEMINATE

	timeout_cnt = 0

	ser.write(str(task_index).encode()) # send commands

	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
			 	print(line)
			 	# if (line == "Input initiator task:"):
			 	#  	time.sleep(2)
			 	#  	print('Input task_index')
			 	# 	ser.write(str(task_index).encode()) # send commands
			 	#	timeout_cnt = 0
			 	if (line == "Waiting for parameter(s)..."):
			 		if(using_patch == 1):
			 			if(daemon_patch == 1):
			 	 			para = "1,0,"+ "%05X" % cbmng_common.get_FileSize(firmware_daemon_burned) + "," + "%05X" % cbmng_common.get_FileSize('patch.bin')
			 			else:
			 	 			para = "1,1,"+ "%05X" % cbmng_common.get_FileSize(firmware_burned) + "," + "%05X" % cbmng_common.get_FileSize('patch.bin')
			 		else:
			 	 		if(daemon_patch == 1):
			 	 			para = "0,0,"+ "%05X" % cbmng_common.get_FileSize(firmware_daemon_burned) + "," + "%05X" % cbmng_common.get_FileSize('patch.bin')
			 	 		else:
			 	 			para = "0,1,"+ "%05X" % cbmng_common.get_FileSize(firmware_burned) + "," + "%05X" % cbmng_common.get_FileSize('patch.bin')
			 		print(para)
			 		ser.write(str(para).encode()) # send commands
			 		timeout_cnt = 0
			 		break
			if(timeout_cnt > 6000):
				break
		except:
			pass
	if(timeout_cnt > 6000):
		print("Timeout...")
		return False

	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
			 	# if (line == "Input initiator task:"):
			 	#  	time.sleep(2)
			 	#  	print('Input task_index')
			 	# 	ser.write(str(task_index).encode()) # send commands
			 	#	timeout_cnt = 0
			 	print(line)
			 	if (line == "C"):
			 	 	print("*YMODEM* send\n")
			 	 	timeout_cnt = 0
			 	 	break
			if(timeout_cnt > 1000):
				break
		except:
			pass
	if(timeout_cnt > 1000):
		print("Timeout...")
		return False
	# transmit the file
	if(using_patch == 1):
		transfer_to_initiator.myserial.serial_send.YMODEM_send('patch.bin')
	else:
		transfer_to_initiator.myserial.serial_send.YMODEM_send(firmware)		
	print("*YMODEM* done\n")

	if(waiting_for_the_execution_timeout(ser, 800) == False): # timeout: 800 seconds
		return False
	
	if(daemon_patch == 1):
		os.system('copy ' + firmware + ' ' + firmware_daemon_burned)
	else:
		os.system('copy ' + firmware + ' ' + firmware_burned)

	print("Done!")
	return True

def waiting_for_the_execution_timeout(ser, timeout_value):
	# Wait for the execution...
	timeout_value = timeout_value * 100
	timeout_cnt = 0
	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
				#print (line)
				if (line == "Input initiator task:"):
	 				timeout_cnt = 0
	 				break
			if(timeout_cnt > timeout_value):
				break
		except:
	 		pass

	if(timeout_cnt > timeout_value):
		print("Timeout...")
		return False

	return True