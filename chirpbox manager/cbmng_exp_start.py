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

""" Md5 """
import hashlib

""" LZSS """
import compression.lzss

import cbmng_exp_globaldef

""" weather """
import chirpbox_weather

exp_conf = "tmp_exp_conf.json"
firmware = "tmp_exp_firm.bin"
firmware_burned = "tmp_exp_firm_burned.bin"
firmware_daemon_burned = "daemon_firm_burned.bin"
exp_meth = "tmp_exp_meth.json"
running_status = "tmp_exp_running.json"


expconfapp = cbmng_exp_config.myExpConfApproach()
expfirmapp = cbmng_exp_firm.myExpFirmwareApproach()
expmethapp = cbmng_exp_method.myExpMethodApproach()
exppriapp = cbmng_exp_config.myExpPrimitiveConf()

EXPERIMENT_START = 0
EXPERIMENT_DISSEMINATE = 1
EXPERIMENT_COLDATA = 2
EXPERIMENT_CONNECT = 3
EXPERIMENT_COLTOPO = 4
EXPERIMENT_COLVER = 5


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
		"experiment_duration": 20,
		"num_generated_packets": "False",
		"num_received_packets": "False",
		"e2e_latency": "False",
		"tx_energy": "False",
		"rx_energy": "False",
		"start_address": "0807E000",
		"end_address": "0807E0D0",
		"command_sf": 12
	}
	with open("tmp.json", "w") as f:
		json.dump(upgrade_dict, f)

def md5(fname):
    hash_md5 = hashlib.md5()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def compression_compare(test_file):
	Demo = compression.lzss.LZSS(7)
	Demo.LZSS_encode(test_file, "encode.bin")
	print(cbmng_common.get_FileSize("encode.bin"), cbmng_common.get_FileSize(test_file))
	if(cbmng_common.get_FileSize("encode.bin") < cbmng_common.get_FileSize(test_file)):
		return True
	else:
		return False

def start(com_port, flash_protection, version_hash, command_sf, bitmap, slot_num, used_tp):
	task_bitmap = "0"
	if(exppriapp.primitive_conf(cbmng_exp_globaldef.exp_primitive_conf) == True):
		print(exppriapp.read_configuration())
	if(expconfapp.experiment_configuration(exp_conf) == True):
		expconfapp.read_configuration()
		# time_now = datetime.datetime.now()
		# start_time_t = time_now + datetime.timedelta(seconds = 60 * 2)
		# start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
		# end_time_t = start_time_t + datetime.timedelta(seconds = expconfapp.experiment_duration)
		# end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
		# exp_no = cbmng_common.tid_maker()
		# exp_name = expconfapp.experiment_name
		# print("Experiment #" + exp_no + " (" + exp_name + ") is going to start at " + start_time + ", and stop at " + end_time)
		# running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': expconfapp.experiment_duration}
		# with open(running_status, "w") as f:
		# 	json.dump(running_dict, f)
	else:
		return False
	print("SF: ", command_sf)
	dissem_back_sf = 0
	dissem_back_slot = 0

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_START

	timeout_cnt = 0
	filename = "start" + datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S") + ".txt"

	with open(filename, 'w+') as f:
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
					print (line)
					if (line == "Input initiator task:"):
						# slot_num = 100
						# ser.write(str(task_index).encode()) # send commands
						task = '{0:01}'.format(int(task_index)) + ',{0:02}'.format(int(command_sf))+ ',{0:03}'.format(int(120)) + ',{0:03}'.format(int(1)) + ',{0:04}'.format(int(slot_num)) + ',{0:02}'.format(int(dissem_back_sf)) + ',{0:03}'.format(int(dissem_back_slot)) + ',{0:02}'.format(int(used_tp)) + "," + '{0:08X}'.format(int(bitmap, 16))+ "," + '{0:08X}'.format(int(task_bitmap, 16))
						print(task)
						ser.write(str(task).encode()) # send commands

						print(datetime.datetime.now())
						f.write(line + "\r")
						line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
						f.write(line_write + "\r")

					if (line == "Waiting for parameter(s)..."):
						time_now = datetime.datetime.now()
						start_time_t = time_now + datetime.timedelta(seconds = 40)
						start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
						end_time_t = start_time_t + datetime.timedelta(seconds = expconfapp.experiment_duration)
						end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
						exp_no = cbmng_common.tid_maker()
						exp_name = expconfapp.experiment_name
						# print("Experiment #" + exp_no + " (" + exp_name + ") is going to start at " + start_time + ", and stop at " + end_time)
						# running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': expconfapp.experiment_duration}
						# with open(running_status, "w") as f:
						# 	json.dump(running_dict, f)

						if(flash_protection == 1):
							para = start_time_t.strftime("%Y,%m,%d,%H,%M,%S") + "," + end_time_t.strftime("%Y,%m,%d,%H,%M,%S") + ",1" + "," + "%04X" % int(version_hash, 16)
						else:
							para = start_time_t.strftime("%Y,%m,%d,%H,%M,%S") + "," + end_time_t.strftime("%Y,%m,%d,%H,%M,%S") + ",0" + "," + "%04X" % int(version_hash, 16)
						print(para)
						ser.write(str(para).encode()) # send commands
						timeout_cnt = 0

						print(datetime.datetime.now())
						f.write(line + "\r")
						line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
						f.write(line_write + "\r")

						break
				if(timeout_cnt > 6000 * 10):
					break
			except:
				pass
	with open(filename, 'a') as f:
		line = "end\r"
		f.write(line)
		line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
		f.write(line_write + "\r")
	if(timeout_cnt > 6000 * 10):
		print("Timeout...")
		return False

	print("Done!")

	# TODO:
	print("Experiment #" + exp_no + " (" + exp_name + ") is going to start at " + start_time + ", and stop at " + end_time)
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 0}
	# with open(running_status, "w") as f:
	# 	json.dump(running_dict, f)

	# filename_log = "log" + datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S") + ".txt"

	# with open(filename_log, 'w+') as f:
	# 	while True:
	# 		try:
	# 			line = ser.readline().decode('ascii').strip() # skip the empty data
	# 			if line:
	# 				print (line)
	# 				if (line.startswith('counter') or line.startswith('reliability') or line.startswith('energest_init')):
	# 					time_now = datetime.datetime.now()
	# 					f.write(line + "\r")
	# 					line_time = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
	# 					f.write(line_time + "\r")
	# 				if (line == "System running from STM32L476 *Bank 2*"):
	# 					time_now = datetime.datetime.now()
	# 					f.write(line + "\r")
	# 					line_time = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
	# 					f.write(line_time + "\r")
	# 				if (line == "System running from STM32L476 *Bank 1*"):
	# 					f.write(line + "\r")
	# 					line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
	# 					f.write(line_write + "\r")
	# 					break
	# 		except:
	# 			pass
	# with open(filename_log, 'a') as f:
	# 	line = "end\r"
	# 	f.write(line)
	# 	line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
	# 	f.write(line_write + "\r")

	# if(waiting_for_the_execution_timeout(ser, 800) == False): # timeout: 800 seconds
	# 	return False

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

def connectivity_evaluation(sf_bitmap, channel, tx_power, command_sf, com_port, slot_num, topo_payload_len, used_tp):
	bitmap = "0"
	task_bitmap = "0"
	dissem_back_sf = 0
	dissem_back_slot = 0

	time_now = datetime.datetime.now()
	start_time_t = time_now + datetime.timedelta(minutes = 2)
	start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
	end_time_t = start_time_t + datetime.timedelta(minutes = 10)
	end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
	exp_no = cbmng_common.tid_maker()
	exp_name = "Chirpbox_connectivity_sf_bitmap" + str(sf_bitmap) + "ch" + str(channel) + "tp" + str(tx_power) + "topo_payload_len" + str(topo_payload_len)
	print("Connectivity evaluation (SF bitmap " + str(sf_bitmap) + ", Channel " + str(channel) + " MHz, " + str(tx_power) + " dBm, "  + "topo_len at " + str(topo_payload_len) + ") is going to start at " + start_time + ", and stop at " + end_time)
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_CONNECT

	timeout_cnt = 0
	filename = "connectivity_evaluation" + datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S") + ".txt"

	with open(filename, 'w+') as f:
		line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
		f.write(line_write + "\r")
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
					print(line)
					if (line == "Input initiator task:"):
						# ser.write(str(task_index).encode()) # send commands
						task = '{0:01}'.format(int(task_index)) + ',{0:02}'.format(int(command_sf))+ ',{0:03}'.format(int(120)) + ',{0:03}'.format(int(1)) + ',{0:04}'.format(int(slot_num)) + ',{0:02}'.format(int(dissem_back_sf)) + ',{0:03}'.format(int(dissem_back_slot)) + ',{0:02}'.format(int(used_tp)) + "," + '{0:08X}'.format(int(bitmap, 16))+ "," + '{0:08X}'.format(int(task_bitmap, 16))
						print(task)
						ser.write(str(task).encode()) # send commands
						print(datetime.datetime.now())
						f.write(line + "\r")
						line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
						f.write(line_write + "\r")
					if (line == "Waiting for parameter(s)..."):
						if(tx_power >= 0):
							para = '{0:02}'.format(int(sf_bitmap)) + ',{0:06}'.format(int(channel)) + ',+{0:02}'.format(int(tx_power)) + ',{0:03}'.format(int(topo_payload_len))
						if(tx_power < 0):
							para = '{0:02}'.format(int(sf_bitmap)) + ',{0:06}'.format(int(channel)) + ',-{0:02}'.format(int(tx_power) * (-1)) + ',{0:03}'.format(int(topo_payload_len))
						print(para)
						ser.write(str(para).encode()) # send commands
						timeout_cnt = 0

						print(datetime.datetime.now())
						f.write(line + "\r")
						line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
						f.write(line_write + "\r")

						break
				if(timeout_cnt > 60000 * 10):
					break
			except:
				pass
	with open(filename, 'a') as f:
		line = "end\r"
		f.write(line)
		line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
		f.write(line_write + "\r")
	if(timeout_cnt > 60000 * 10):
		print("Timeout...")
		return False

	if(waiting_for_the_execution_timeout(ser, 12000) == False): # timeout: 800 seconds
		return False

	time_now1 = datetime.datetime.now()
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': time_now1.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)
	print("Done!")
	# if(waiting_for_the_execution_timeout(ser, 12000) == False): # timeout: 800 seconds
	# 	return False

	# save weather data
	weather = chirpbox_weather.testbed_weather()
	weather_json_object = json.dumps(weather.weather_current())
	with open(filename[:-4]+".json", "w") as outfile:
		outfile.write(weather_json_object)

	return True

def collect_data(com_port, command_len, command_sf, slot_num, used_tp, task_bitmap, start_address_col, end_address_col):
	bitmap = "0"
	dissem_back_sf = 0
	dissem_back_slot = 0

	if(expmethapp.experiment_methodology(exp_meth) == True):
		expmethapp.read_configuration()
		start_addr = expmethapp.start_address
		end_addr = expmethapp.end_address
		print("start address: " + str(start_addr))
		print("end address: " + str(end_addr))
	else:
		return False
	if(expconfapp.experiment_configuration(exp_conf) == True):
		expconfapp.read_configuration()
	else:
		return False

	with open(running_status,'r') as load_f:
		load_dict = json.load(load_f)
		filename = load_dict['exp_name'] +"(" + load_dict['exp_number'] + ").txt"

	time_now = datetime.datetime.now()
	start_time_t = time_now + datetime.timedelta(minutes = 0)
	start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
	end_time_t = start_time_t + datetime.timedelta(minutes = 0)
	end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
	exp_no = cbmng_common.tid_maker()

	exp_name = "collect_data_command_len_" + str(command_len) + "_used_sf" + str(command_sf) + "used_tp" + str(used_tp) + "command_len" + str(command_len) + "_slot_num" + str(slot_num) + "startaddress_" + str(start_address_col) + "end_address" + str(end_address_col)
	print(exp_name)
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)

	print("Collecting ...")

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_COLDATA

	timeout_cnt = 0

	filename_1 = "collect_data" + datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S") + "_used_sf" + str(command_sf) + "_task_bitmap" + str(task_bitmap) + "_addr"+str(start_address_col)+"_"+str(end_address_col) + ".txt"

	with open(filename_1, 'w+') as f_1:
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
					print (line)
					if (line == "Input initiator task:"):
						task = '{0:01}'.format(int(task_index)) + ',{0:02}'.format(int(command_sf))+ ',{0:03}'.format(int(command_len)) + ',{0:03}'.format(int(1)) + ',{0:04}'.format(int(slot_num)) + ',{0:02}'.format(int(dissem_back_sf)) + ',{0:03}'.format(int(dissem_back_slot)) + ',{0:02}'.format(int(used_tp)) + "," + '{0:08X}'.format(int(bitmap, 16)) + "," + '{0:08X}'.format(int(task_bitmap, 16))
						print(task)
						ser.write(str(task).encode()) # send commands
						print(datetime.datetime.now())
						f_1.write(line + "\r")
						line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
						f_1.write(line_write + "\r")

					if (line == "Waiting for parameter(s)..."):
						para = "%08X" % int(start_address_col, 16) + "," + "%08X" % int(end_address_col, 16)
						print(para)
						ser.write(str(para).encode()) # send commands
						timeout_cnt = 0
						print(datetime.datetime.now())
						f_1.write(line + "\r")
						line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
						f_1.write(line_write + "\r")

						break
				if(timeout_cnt > 60000 * 30):
					break
			except:
				pass
	if(timeout_cnt > 60000 * 30):
		print("Timeout...")
		return False

	with open(filename, 'w+') as f:
		start_read = 0
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
					print (line)
					if (line == "output from initiator (collect):"):
						start_read = 1
					if (line == "---------MX_GLOSSY---------"):
						timeout_cnt = 0
						break
					if(start_read == 1):
						f.write(line + "\r")
				if(timeout_cnt > 60000 * 30):
					break
			except:
				pass
	if(timeout_cnt > 60000 * 30):
		print("Timeout...")
		return False
	with open(filename_1, 'a') as f_1:
		line = "end\r"
		f_1.write(line)
		line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
		f_1.write(line_write + "\r")
	print("Results of " + filename + " have been collected!" )
	return True


def collect_topology(com_port, command_sf, command_len, slot_num, used_tp):
	bitmap = "0"
	task_bitmap = "0"
	dissem_back_sf = 0
	dissem_back_slot = 0

	with open(running_status,'r') as load_f:
		load_dict = json.load(load_f)
		filename = "chirpbox_topo//" + load_dict['exp_name'] +"(" + load_dict['exp_number'] + ").txt"
	print("Collecting ...")

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_COLTOPO

	timeout_cnt = 0
	filename_1 = "collect_topology" + datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S") + ".txt"
	with open(filename_1, 'w+') as f_1:

		with open(filename, 'w+') as f:
			start_read = 0
			while True:
				try:
					line = ser.readline().decode('ascii').strip() # skip the empty data
					timeout_cnt = timeout_cnt + 1
					if line:
						print (line)
						if (line == "Input initiator task:"):
							# ser.write(str(task_index).encode()) # send commands
							# print(task_index)
							task = '{0:01}'.format(int(task_index)) + ',{0:02}'.format(int(command_sf))+ ',{0:03}'.format(int(command_len)) + ',{0:03}'.format(int(1)) + ',{0:04}'.format(int(slot_num)) + ',{0:02}'.format(int(dissem_back_sf)) + ',{0:03}'.format(int(dissem_back_slot)) + ',{0:02}'.format(int(used_tp)) + "," + '{0:08X}'.format(int(bitmap, 16))+ "," + '{0:08X}'.format(int(task_bitmap, 16))
							print(task)
							ser.write(str(task).encode()) # send commands
							print(datetime.datetime.now())
							f_1.write(line + "\r")
							line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
							f_1.write(line_write + "\r")
							# break

						if (line == "output from initiator (topology):"):
							start_read = 1
						if ((line == "---------MX_GLOSSY---------") and (start_read == 1)):
							timeout_cnt = 0
							break
						if(start_read == 1):
							f.write(line + "\r")
					if(timeout_cnt > 60000 * 20):
						break
				except:
					pass
			f.close()
	if(waiting_for_the_execution_timeout(ser, 60000 * 20) == False): # timeout: 800 seconds
		return False

	if(timeout_cnt > 60000 * 20):
	 	print("Timeout...")
	 	return False
	with open(filename_1, 'a') as f_1:
		line = "end\r"
		f_1.write(line)
		line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
		f_1.write(line_write + "\r")
	print("Results of " + filename + " have been collected!" )
	# # filename = "Chirpbox_connectivity_sf7ch470000tp0(20200801141355709008).txt"
	# results = statistics_process.topo_parser.topo_parser(filename, using_pos)
	# # max_hop, mean_degree_array, std_dev_degree_array, min_dev_degree_array, max_dev_degree_array
	# print("Max_hop: " + str(results[0]))
	# print("Mean_degree: " + str(results[1]))
	# print("Std_dev_degree: " + str(results[2]))
	# print("Min_degree: " + str(results[3]))
	# print("Max_degree: " + str(results[4]))
	# print("Symmetry: " + str(results[5]))
	# print("Temperature of nodes: " + str(results[6]))
	# filename_hop = "hop_" + filename
	# with open(filename_hop, 'w+') as f:
	# 	line = ("Max_hop: " + str(results[0]))
	# 	f.write(line + "\r")
	# 	line = ("Mean_degree: " + str(results[1]))
	# 	f.write(line + "\r")
	# 	line = ("Std_dev_degree: " + str(results[2]))
	# 	f.write(line + "\r")
	# 	line = ("Min_degree: " + str(results[3]))
	# 	f.write(line + "\r")
	# 	line = ("Max_degree: " + str(results[4]))
	# 	f.write(line + "\r")
	# 	line = ("Symmetry: " + str(results[5]))
	# 	f.write(line + "\r")
	# 	line = ("Temperature of nodes: " + str(results[6]))
	# 	f.write(line + "\r")
	return True


def collect_version(com_port, command_sf, slot_num, used_tp):
	bitmap = "0"
	task_bitmap = "0"
	filename = "version" + datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S") + ".txt"
	dissem_back_sf = 0
	dissem_back_slot = 0

	time_now = datetime.datetime.now()
	start_time_t = time_now + datetime.timedelta(minutes = 0)
	start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
	end_time_t = start_time_t + datetime.timedelta(minutes = 0)
	end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
	exp_no = cbmng_common.tid_maker()

	FileSize = cbmng_common.get_FileSize(firmware)
	exp_name = "version" + "_used_sf" + str(command_sf) + "_slot_num" + str(slot_num) + "_payload_len_11"
	print(exp_name)
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_COLVER

	timeout_cnt = 0
	input = 0

	with open(filename, 'w+') as f:
		start_read = 0
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
				 	print (line)
				 	if (line == "Input initiator task:"):
				 		input = 1
				 		print("Input")
				 		task = '{0:01}'.format(int(task_index)) + ',{0:02}'.format(int(command_sf))+ ',{0:03}'.format(int(120)) + ',{0:03}'.format(int(1)) + ',{0:04}'.format(int(slot_num)) + ',{0:02}'.format(int(dissem_back_sf)) + ',{0:03}'.format(int(dissem_back_slot)) + ',{0:02}'.format(int(used_tp)) + "," + '{0:08X}'.format(int(bitmap, 16))+ "," + '{0:08X}'.format(int(task_bitmap, 16))
				 		print(task)
				 		ser.write(str(task).encode()) # send commands
				 		# ser.write(str(task_index).encode()) # send commands
				 	if (line == "output from initiator (version):"):
				 		start_read = 1
	 				if ((line == "---------MX_GLOSSY---------") and (input == 1)):
	 					timeout_cnt = 0
	 					input = 0
	 					break
	 				if(start_read == 1):
	 					f.write(line + "\r")
				if(timeout_cnt > 60000 * 3):
	 				break
			except:
	 			pass
	if(timeout_cnt > 60000 * 3):
	 	print("Timeout...")
	 	return False

	print("Version has been collected!" )
	return True


def disseminate(com_port, daemon_patch, version_hash, command_len, command_sf, command_size, bitmap, slot_num, dissem_back_sf, dissem_back_slot, used_tp, task_bitmap):
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

	if(expconfapp.experiment_configuration(exp_conf) == True):
		expconfapp.read_configuration()
	else:
		return False

	time_now = datetime.datetime.now()
	start_time_t = time_now + datetime.timedelta(minutes = 0)
	start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
	end_time_t = start_time_t + datetime.timedelta(minutes = 0)
	end_time = end_time_t.strftime("%Y-%m-%d %H:%M:%S")
	exp_no = cbmng_common.tid_maker()

	FileSize = cbmng_common.get_FileSize(firmware)
	exp_name = "disseminate_command_len_" + str(command_len) + "_used_sf" + str(command_sf) + "used_tp" + str(used_tp) + "_generate_size" + str(command_size) + "_slot_num" + str(slot_num) + "_bitmap" + str(task_bitmap) + "_FileSize" + str(FileSize) + "_dissem_back_sf" + str(dissem_back_sf) + "_dissem_back_slot" + str(dissem_back_slot)
	print(exp_name)
	running_dict = {'exp_name': exp_name, 'exp_number': exp_no, 'start_time': time_now.strftime("%Y-%m-%d %H:%M:%S"), 'end_time': end_time_t.strftime("%Y-%m-%d %H:%M:%S"), 'duration': 10}
	with open(running_status, "w") as f:
		json.dump(running_dict, f)

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

	using_compression = 0
	if(using_patch):
		using_compression = compression_compare("patch.bin")
	else:
		using_compression = compression_compare(firmware)
	if(using_compression == False):
		using_compression = 0
	else:
		using_compression = 1
	print("using_compression:", using_compression)

	# config and open the serial port
	ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)
	# corresponded task number
	task_index = EXPERIMENT_DISSEMINATE

	timeout_cnt = 0

	hash_md5 = md5(firmware)
	print("hash_md5:", "%16X" % int(hash_md5, 16))

	filename_1 = "disseminate" + datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S") + "_used_sf" + str(command_sf) + "_task_bitmap" + str(task_bitmap)+"_size"+str(FileSize) + ".txt"
	with open(filename_1, 'w+') as f_1:

		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
					print(line)
					if (line == "Input initiator task:"):
						# TODO:
						task = '{0:01}'.format(int(task_index)) + ',{0:02}'.format(int(command_sf)) + ',{0:03}'.format(int(command_len)) + ',{0:03}'.format(int(command_size)) + ',{0:04}'.format(int(slot_num)) + ',{0:02}'.format(int(dissem_back_sf)) + ',{0:03}'.format(int(dissem_back_slot)) + ',{0:02}'.format(int(used_tp)) + "," + '{0:08X}'.format(int(bitmap, 16)) + "," + '{0:08X}'.format(int(task_bitmap, 16))
						print(task)
						ser.write(str(task).encode()) # send commands
						# print(task)
						print(datetime.datetime.now())
						f_1.write(line + "\r")
						line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
						f_1.write(line_write + "\r")

					if (line == "Waiting for parameter(s)..."):
						if(using_patch == 1):
							if(daemon_patch == 1):
								para = "1,0,"+'{0:01},'.format(int(using_compression))+"%05X" % cbmng_common.get_FileSize(firmware_daemon_burned) + "," + "%05X" % cbmng_common.get_FileSize('patch.bin') + "," + "%04X" % int(version_hash, 16) + "," + "%32X" % int(hash_md5, 16)
							else:
								para = "1,1,"+'{0:01},'.format(int(using_compression))+ "%05X" % cbmng_common.get_FileSize(firmware_burned) + "," + "%05X" % cbmng_common.get_FileSize('patch.bin') + "," + "%04X" % int(version_hash, 16) + "," + "%32X" % int(hash_md5, 16)
						else:
							if(daemon_patch == 1):
								para = "0,0,"+'{0:01},'.format(int(using_compression))+ "%05X" % cbmng_common.get_FileSize(firmware_daemon_burned) + "," + "%05X" % cbmng_common.get_FileSize('patch.bin') + "," + "%04X" % int(version_hash, 16) + "," + "%32X" % int(hash_md5, 16)
							else:
								para = "0,1,"+'{0:01},'.format(int(using_compression))+ "%05X" % cbmng_common.get_FileSize(firmware_burned) + "," + "%05X" % cbmng_common.get_FileSize('patch.bin') + "," + "%04X" % int(version_hash, 16) + "," + "%32X" % int(hash_md5, 16)
						print(para)
						ser.write(str(para).encode()) # send commands
						timeout_cnt = 0
						print(datetime.datetime.now())
						f_1.write(line + "\r")
						line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
						f_1.write(line_write + "\r")

						break
				if(timeout_cnt > 60000 * 3):
					break
			except:
				pass
	if(timeout_cnt > 60000 * 3):
		print("Timeout...")
		return False

	while True:
		try:
			line = ser.readline().decode('ascii').strip() # skip the empty data
			timeout_cnt = timeout_cnt + 1
			if line:
				print(line)
				if (line == "C"):
					print("*YMODEM* send\n")
					timeout_cnt = 0
					break
			if(timeout_cnt > 6000 * 10):
				break
		except:
			pass
	if(timeout_cnt > 6000 * 10):
		print("Timeout...")
		return False
	# transmit the file
	YMODEM_result = False
	if(using_patch == 1):
		while(YMODEM_result != True):
			if(using_compression):
				YMODEM_result = transfer_to_initiator.myserial.serial_send.YMODEM_send("encode.bin")
			else:
				YMODEM_result = transfer_to_initiator.myserial.serial_send.YMODEM_send('patch.bin')
	else:
		while(YMODEM_result != True):
			if(using_compression):
				YMODEM_result = transfer_to_initiator.myserial.serial_send.YMODEM_send("encode.bin")
			else:
				YMODEM_result = transfer_to_initiator.myserial.serial_send.YMODEM_send(firmware)
	print("*YMODEM* done\n")

	if(waiting_for_the_execution_timeout(ser, 60000 * 20) == False): # timeout: 800 seconds
		return False

	if(daemon_patch == 1):
		os.system('copy ' + firmware + ' ' + firmware_daemon_burned)
	else:
		os.system('copy ' + firmware + ' ' + firmware_burned)

	print("Done!")
	with open(filename_1, 'a') as f_1:
		line = "end\r"
		f_1.write(line)
		line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
		f_1.write(line_write + "\r")
	# if(waiting_for_the_execution_timeout(ser, 360) == False): # timeout: 800 seconds
	# 	return False

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
				print (line)
				if (line == "---------MX_GLOSSY---------"):
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