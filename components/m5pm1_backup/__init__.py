import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.components import sensor
from esphome.components import binary_sensor
from esphome.const import *

DEPENDENCIES = ["i2c"]

m5pm1_ns = cg.esphome_ns.namespace("m5pm1")
M5PM1Component = m5pm1_ns.class_("M5PM1Component", cg.PollingComponent, i2c.I2CDevice)

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(M5PM1Component),
        cv.Optional("update_interval", default="60s"): cv.update_interval,
        cv.Optional("battery_voltage"): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
        ),
        cv.Optional("on_battery"): binary_sensor.binary_sensor_schema(),
        cv.Optional("on_usb"): binary_sensor.binary_sensor_schema(),
    }).extend(i2c.i2c_device_schema(0x6e))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    
    if update_interval := config.get("update_interval"):
        cg.add(var.set_update_interval(update_interval))

    if voltage := config.get("battery_voltage"):
        sens = await sensor.new_sensor(voltage)
        cg.add(var.set_battery_voltage_sensor(sens))

    if bat := config.get("on_battery"):
        bs = await binary_sensor.new_binary_sensor(bat)
        cg.add(var.set_on_battery_sensor(bs))

    if usb := config.get("on_usb"):
        bs = await binary_sensor.new_binary_sensor(usb)
        cg.add(var.set_on_usb_sensor(bs))