import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_TYPE

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1TextSensor = m5pm1_ns.class_(
    "M5PM1TextSensor", text_sensor.TextSensor, cg.PollingComponent
)
TextSensorType = m5pm1_ns.enum("M5PM1TextSensorType")

TYPES = {
    "power_source": TextSensorType.TEXT_POWER_SOURCE,
    "wake_source": TextSensorType.TEXT_WAKE_SOURCE,
}

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(M5PM1TextSensor)
    .extend(
        {
            cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component),
            cv.Required(CONF_TYPE): cv.enum(TYPES, lower=True),
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(config[CONF_TYPE]))
