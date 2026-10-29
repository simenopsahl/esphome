import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_NUM_LEDS, CONF_OUTPUT_ID

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1Light = m5pm1_ns.class_("M5PM1Light", light.AddressableLight)

CONFIG_SCHEMA = cv.All(
    light.ADDRESSABLE_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(M5PM1Light),
            cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component),
            cv.Optional(CONF_NUM_LEDS, default=1): cv.int_range(min=1, max=32),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_num_leds(config[CONF_NUM_LEDS]))
