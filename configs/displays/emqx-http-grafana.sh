SELECT
  payload.params.temperature.value as temperature,
  payload.params.humidity.value as humidity,
  payload.params.pressure.value as pressure,
  coalesce(payload.params.tvoc.value, 0.0) as tvoc,
  coalesce(payload.params.formaldehyde.value, 0.0) as formaldehyde,
  coalesce(payload.params.co2.value, 0.0) as co2,
  coalesce(payload.params.temperature.time, timestamp) as timestamp,
  clientid as device_id
FROM
  "mypanel/thing/property/post"

------------------------------------------------------------------

{
  "resourceMetrics": [
    {
      "scopeMetrics": [
        {
          "metrics": [
            {
              "name": "temperature_celsius",
              "unit": "°C",
              "description": "Environment temperature",
              "gauge": {
                "dataPoints": [
                  {
                    "asDouble": ${temperature},
                    "timeUnixNano": ${timestamp},
                    "attributes": [
                      { "key": "device_id", "value": { "stringValue": "${device_id}" } }
                    ]
                  }
                ]
              }
            },
            {
              "name": "humidity_percent",
              "unit": "%",
              "description": "Relative humidity",
              "gauge": {
                "dataPoints": [
                  {
                    "asDouble": ${humidity},
                    "timeUnixNano": ${timestamp},
                    "attributes": [
                      { "key": "device_id", "value": { "stringValue": "${device_id}" } }
                    ]
                  }
                ]
              }
            },
            {
              "name": "pressure_hpa",
              "unit": "hPa",
              "description": "Air pressure",
              "gauge": {
                "dataPoints": [
                  {
                    "asDouble": ${pressure},
                    "timeUnixNano": ${timestamp},
                    "attributes": [
                      { "key": "device_id", "value": { "stringValue": "${device_id}" } }
                    ]
                  }
                ]
              }
            },
            {
              "name": "co2_ppm",
              "unit": "ppm",
              "description": "CO2 concentration",
              "gauge": {
                "dataPoints": [
                  {
                    "asDouble": ${co2},
                    "timeUnixNano": ${timestamp},
                    "attributes": [
                      { "key": "device_id", "value": { "stringValue": "${device_id}" } }
                    ]
                  }
                ]
              }
            },
            {
              "name": "tvoc_ppm",
              "unit": "ppm",
              "description": "Total Volatile Organic Compounds",
              "gauge": {
                "dataPoints": [
                  {
                    "asDouble": ${tvoc},
                    "timeUnixNano": ${timestamp},
                    "attributes": [
                      { "key": "device_id", "value": { "stringValue": "${device_id}" } }
                    ]
                  }
                ]
              }
            },
            {
              "name": "formaldehyde_ppm",
              "unit": "ppm",
              "description": "Formaldehyde concentration",
              "gauge": {
                "dataPoints": [
                  {
                    "asDouble": ${formaldehyde},
                    "timeUnixNano": ${timestamp},
                    "attributes": [
                      { "key": "device_id", "value": { "stringValue": "${device_id}" } }
                    ]
                  }
                ]
              }
            }
          ]
        }
      ]
    }
  ]
}
