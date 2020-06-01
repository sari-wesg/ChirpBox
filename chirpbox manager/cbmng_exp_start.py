import cbmng_exp_config
import cbmng_exp_firm
import cbmng_exp_method
import cbmng_common
import datetime
import time
import json
import transfer_to_initiator.myserial.serial_send

import serial

exp_conf = "tmp_exp_conf.json"
firmware = "tmp_exp_firm.bin"
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

def start(com_port):
	if(expconfapp.experiment_configuration(exp_conf) == True):
		expconfapp.read_configuration()
		time_now = datetime.datetime.now()
		start_time_t = time_now + datetime.timedelta(minutes = 2)
		start_time = start_time_t.strftime("%Y-%m-%d %H:%M:%S")
		end_time_t = start_time_t + datetime.timedelta(minutes = expconfapp.experiment_duration)
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
			 		para = start_time_t.strftime("%Y,%m,%d,%H,%M,%S") + "," + end_time_t.strftime("%Y,%m,%d,%H,%M,%S")
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

	print("Done!")
	return True

def assign_sniffer(num, pairs, com_port):
	if(expmethapp.experiment_methodology(exp_meth) == True):
		expmethapp.read_configuration()
		time_now = datetime.datetime.now()
		print("The sniffer(s): " + str(expmethapp.sniffer))
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
			 	 		print("There is " + str(num) + " sniffers.")
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

def collect_data(com_port, start_addr, end_addr):
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
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
					# if (line == "Input initiator task:"):
				 	#  	time.sleep(2)
				 	#  	print('Input task_index')
					#   ser.write(str(task_index).encode()) # send commands
					if (line == "Task list:"):
						timeout_cnt = 0
						break
					# print(line)
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

def collect_topology(com_port):
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
		while True:
			try:
				line = ser.readline().decode('ascii').strip() # skip the empty data
				timeout_cnt = timeout_cnt + 1
				if line:
					
				 	# if (line == "Input initiator task:"):
				 	#  	time.sleep(2)
				 	#  	print('Input task_index')
					#   ser.write(str(task_index).encode()) # send commands
					if (line == "Task list:"):
						timeout_cnt = 0
						break
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

def disseminate(com_port):
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
			 	# if (line == "Input initiator task:"):
			 	#  	time.sleep(2)
			 	#  	print('Input task_index')
			 	# 	ser.write(str(task_index).encode()) # send commands
			 	#	timeout_cnt = 0
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
	transfer_to_initiator.myserial.serial_send.YMODEM_send(firmware)
	print("*YMODEM* done\n")

	if(waiting_for_the_execution_timeout(ser, 800) == False): # timeout: 800 seconds
		return False

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