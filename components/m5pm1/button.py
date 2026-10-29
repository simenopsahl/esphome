import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_TYPE

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1Button = m5pm1_ns.class_("M5PM1Button", button.Button, cg.Component)
ButtonType = m5pm1_ns.enum("M5PM1ButtonType")

TYPES = {
    "shutdown": ButtonType.BUTTON_SHUTDOWN,
    "reboot": ButtonType.BUTTON_REBOOT,
    "feed_watchdog": ButtonType.BUTTON_FEED_WATCHDOG,
    "refresh_leds": ButtonType.BUTTON_REFRESH_LEDS,
    "enter_download_mode": ButtonType.BUTTON_ENTER_DOWNLOAD_MODE,
    "timer_clear": ButtonType.BUTTON_TIMER_CLEAR,
}

CONFIG_SCHEMA = (
    button.button_schema(M5PM1Button)
    .extend(
        {
            cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component),
            cv.Required(CONF_TYPE): cv.enum(TYPES, lower=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(config[CONF_TYPE]))
