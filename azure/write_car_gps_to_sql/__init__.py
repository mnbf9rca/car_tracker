

import logging
from collections.abc import Iterable
from json import dumps, loads
from os import environ

import azure.functions as func
import pyodbc
from dateutil import parser

logger = logging.getLogger(f"azure.func.{__name__}")

def main(event: func.EventHubEvent):

    # for some reason, when executing in azure, we dont always get an iterable, even with cardinality=many
    # so test for it
    if isinstance(event, Iterable):
        logger.debug('iterable')
        event_data = [loads(s) for s in [e.get_body().decode('utf-8') for e in event]]
    else:
        logger.debug('not iterable')
        event_data = loads(event.get_body().decode('utf-8'))
    logger.info(f'Received trigger for {len(event_data)} items')
    logger.debug(f'payload: {dumps(event_data)}')

    return _write_to_database(event_data)

def _write_to_database(event_data: dict) -> int:
    try:
        cnxn = pyodbc.connect(environ.get('cloud_sql_conn_string'))
    except Exception as e:
            logger.error(e)
    try:
        with cnxn:
            cursor = cnxn.cursor()
            cursor.fast_executemany = True
            ''' 
            sp: insert_car_measurement
            @event
            @latlong
            @published_at
            @deviceID

            message:
            {
            "device_id": "{{{PARTICLE_DEVICE_ID}}}",
            "published_at": "{{{PARTICLE_PUBLISHED_AT}}}",
            "source": "particle.io",
            "type": "car_gps",
            "name": "car_gps",
            "event": "{{{PARTICLE_EVENT_NAME}}}",
            "data": "{{{PARTICLE_EVENT_VALUE}}}"
            }
            '''

            cursor.executemany("{CALL insert_car_measurement (?,?,?,?)}", 
                                    [(s["event"], s["data"],  parser.parse(s["published_at"], s["device_id"]))
                                    for s in event_data])
    except Exception as e:
        logger.error(f'Error received from DB commit: {e}')
        return -1
    # shouldnt need to close, but seems to improve throughput
    cnxn.close()
    logger.info(f"Successfully stored {len(event_data)} entries in DB")
    return None