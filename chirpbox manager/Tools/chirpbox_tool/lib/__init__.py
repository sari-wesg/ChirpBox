import logging

logger = logging.getLogger(__name__)
logger.propagate = False
logger.setLevel(logging.INFO) # <<< Added Line
logging.getLogger('matplotlib').setLevel(logging.ERROR)
logging.getLogger('matplotlib.font_manager').disabled = True
logging.getLogger('matplotlib.colorbar').disabled = True

# Log config:
# Create handlers
c_handler = logging.StreamHandler()
c_handler.setLevel(logging.DEBUG)

# Create formatters and add it to handlers
c_format = logging.Formatter('[%(filename)s:%(lineno)d] %(asctime)s - %(name)s - %(levelname)s - %(message)s')
c_handler.setFormatter(c_format)

# Add handlers to the logger
logger.addHandler(c_handler)
