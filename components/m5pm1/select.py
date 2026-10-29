import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1I2CSpeedSelect = m5pm1_ns.class_(
    "M5PM1I2CSpeedSelect", select.Select, cg.Component
)

OPTIONS = ["100kHz", "400kHz"]

CONFIG_SCHEMA = select.select_schema(M5PM1I2CSpeedSelect).extend(
    {
        cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component),
    }
)


async def to_code(config):
    var = await select.new_select(config, options=OPTIONS)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
