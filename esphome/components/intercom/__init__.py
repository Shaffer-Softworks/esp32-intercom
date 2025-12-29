import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, text_sensor, switch
from esphome.const import CONF_ID

CODEOWNERS = ["@michaelshaffer"]
DEPENDENCIES = ["wifi"]

intercom_ns = cg.esphome_ns.namespace("intercom")
IntercomComponent = intercom_ns.class_("IntercomComponent", cg.Component)

CONF_INTERCOM = "intercom"
CONF_SIGNALING_SERVER = "signaling_server"
CONF_SIGNALING_PORT = "signaling_port"
CONF_SIGNALING_PATH = "signaling_path"
CONF_CLIENT_ID_PREFIX = "client_id_prefix"
CONF_CALL_STATE = "call_state"
CONF_CALL_STATUS = "call_status"
CONF_START_CALL = "start_call"
CONF_END_CALL = "end_call"
CONF_ACCEPT_CALL = "accept_call"
CONF_MUTE = "mute"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(IntercomComponent),
    cv.Required(CONF_SIGNALING_SERVER): cv.string,
    cv.Optional(CONF_SIGNALING_PORT, default=1880): cv.port,
    cv.Optional(CONF_SIGNALING_PATH, default="/endpoint/webrtc"): cv.string,
    cv.Optional(CONF_CLIENT_ID_PREFIX, default="esphome-"): cv.string,
    cv.Optional(CONF_CALL_STATE): sensor.sensor_schema(
        unit_of_measurement="",
        accuracy_decimals=1,
    ),
    cv.Optional(CONF_CALL_STATUS): text_sensor.text_sensor_schema(),
    cv.Optional(CONF_START_CALL): switch.switch_schema(),
    cv.Optional(CONF_END_CALL): switch.switch_schema(),
    cv.Optional(CONF_ACCEPT_CALL): switch.switch_schema(),
    cv.Optional(CONF_MUTE): switch.switch_schema(),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_signaling_server(config[CONF_SIGNALING_SERVER]))
    cg.add(var.set_signaling_port(config[CONF_SIGNALING_PORT]))
    cg.add(var.set_signaling_path(config[CONF_SIGNALING_PATH]))
    cg.add(var.set_client_id_prefix(config[CONF_CLIENT_ID_PREFIX]))
    
    if CONF_CALL_STATE in config:
        sens = await sensor.new_sensor(config[CONF_CALL_STATE])
        cg.add(var.set_call_state_sensor(sens))
    
    if CONF_CALL_STATUS in config:
        text_sens = await text_sensor.new_text_sensor(config[CONF_CALL_STATUS])
        cg.add(var.set_call_status_text_sensor(text_sens))
    
    if CONF_START_CALL in config:
        sw = await switch.new_switch(config[CONF_START_CALL])
        cg.add(var.set_start_call_switch(sw))
    
    if CONF_END_CALL in config:
        sw = await switch.new_switch(config[CONF_END_CALL])
        cg.add(var.set_end_call_switch(sw))
    
    if CONF_ACCEPT_CALL in config:
        sw = await switch.new_switch(config[CONF_ACCEPT_CALL])
        cg.add(var.set_accept_call_switch(sw))
    
    if CONF_MUTE in config:
        sw = await switch.new_switch(config[CONF_MUTE])
        cg.add(var.set_mute_switch(sw))

