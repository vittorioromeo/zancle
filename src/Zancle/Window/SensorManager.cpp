// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/SensorManager.hpp"

#include "Zancle/Err/Err.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
bool SensorManager::isAvailable(Sensor::Type sensor)
{
    return m_sensors[sensor].available;
}


////////////////////////////////////////////////////////////
void SensorManager::setEnabled(Sensor::Type sensor, bool enabled)
{
    if (m_sensors[sensor].available)
    {
        m_sensors[sensor].enabled = enabled;
        m_sensors[sensor].sensor.setEnabled(enabled);
    }
    else
    {
        priv::errMsg("Warning: trying to enable a sensor that is not available (call Sensor::isAvailable to check it)");
    }
}


////////////////////////////////////////////////////////////
bool SensorManager::isEnabled(Sensor::Type sensor) const
{
    return m_sensors[sensor].enabled;
}


////////////////////////////////////////////////////////////
Vec3f SensorManager::getValue(Sensor::Type sensor) const
{
    return m_sensors[sensor].value;
}


////////////////////////////////////////////////////////////
void SensorManager::update()
{
    for (Item& item : m_sensors.elements)
    {
        // Only process available sensors
        if (item.available)
            item.value = item.sensor.update();
    }
}


////////////////////////////////////////////////////////////
SensorManager::SensorManager()
{
    // Global sensor initialization
    SensorImpl::initialize();

    // Per sensor initialization
    for (unsigned int i = 0; i < Sensor::Count; ++i)
    {
        const auto sensor = static_cast<Sensor::Type>(i);

        // Check which sensors are available
        m_sensors[sensor].available = SensorImpl::isAvailable(sensor);

        // Open the available sensors
        if (m_sensors[sensor].available)
        {
            if (m_sensors[sensor].sensor.open(sensor))
            {
                m_sensors[sensor].sensor.setEnabled(false);
            }
            else
            {
                m_sensors[sensor].available = false;
                priv::errMsg("Warning: sensor {} failed to open, will not be available", i);
            }
        }
    }
}


////////////////////////////////////////////////////////////
SensorManager::~SensorManager()
{
    // Per sensor cleanup
    for (Item& item : m_sensors.elements)
    {
        if (item.available)
            item.sensor.close();
    }

    // Global sensor cleanup
    SensorImpl::cleanup();
}

} // namespace za::priv
