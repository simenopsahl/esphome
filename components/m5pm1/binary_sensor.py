import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_TYPE

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1BinarySensor = m5pm1_ns.class_(
    "M5PM1BinarySensor", binary_sensor.BinarySensor, cg.PollingComponent
)
BinarySensorType = m5pm1_ns.enum("M5PM1BinarySensorType")

TYPES = {
    "button": BinarySensorType.BINARY_BUTTON,
    "external_power": BinarySensorType.BINARY_EXTERNAL_POWER,
}

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(M5PM1BinarySensor)
    .extend(
        {
            cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component),
            cv.Required(CONF_TYPE): cv.enum(TYPES, lower=True),
        }
    )
    .extend(cv.polling_component_schema("10s"))
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(config[CONF_TYPE]))
