import pika
import argparse
import json
import threading
import yaml
import logging
import datetime
from datetime import datetime

def process_signal_thread(conf, sig, source_exchange, source_routing_key, target_exchange, target_routing_key,
                          target_json_pack, target_json_path, source_datatype, target_datatype):
    in_server_user = conf['servers']['source']['user']
    in_server_password = conf['servers']['source']['password']
    in_server_port = conf['servers']['source']['port']
    in_server_host = conf['servers']['source']['host']

    out_server_user = conf['servers']['target']['user']
    out_server_password = conf['servers']['target']['password']
    out_server_port = conf['servers']['target']['port']
    out_server_host = conf['servers']['target']['host']

    in_credentials = pika.PlainCredentials(in_server_user, in_server_password)
    in_parameters = pika.ConnectionParameters(in_server_host, in_server_port, '/', in_credentials)
    in_connection = pika.BlockingConnection(in_parameters)
    in_channel = in_connection.channel()
    logging.info("In channel opened for {0}:{1}".format(in_server_host, in_server_port))

    out_credentials = pika.PlainCredentials(out_server_user, out_server_password)
    out_parameters = pika.ConnectionParameters(out_server_host, out_server_port, '/', out_credentials)
    out_connection = pika.BlockingConnection(out_parameters)
    out_channel = out_connection.channel()
    logging.info("In channel opened for {0}:{1}".format(out_server_host, out_server_port))

    # declare out exchanges
    target_exchanges = {conf['signals'][s]['target']['exchange'] for s in conf['signals'].keys()}
    for exchange in target_exchanges:
        out_channel.exchange_declare(exchange=exchange, exchange_type='direct')

    def callback(ch, method, properties, body):

        # check if we know how to decode time otherwise insert time of arrival here
        # if not isinstance(body, str):
        #     logging.warning("Cannot decode message of type: " + type(body))
        #     return
        logging.warning("Message: " + str(body))
        if source_datatype == "double":
            data = float(body)
        elif source_datatype == "int":
            data = int(body)
        elif source_datatype == "boolean":
            data = bool(body)
        elif source_datatype == "string":
            data = bytes.decode(body, 'utf-8')

        if target_datatype == "double":
            data = float(data)
        elif target_datatype == "int":
            data = int(data)
        elif target_datatype == "boolean":
            data = bool(data)
        elif target_datatype == "string":
            data = str(data)

        time_stamp = datetime.now().astimezone().isoformat()

        if target_json_pack:
            # check to try and see what happens if data is always there
            dd={'level':0.0,'valve':0.0}
            d={target_json_path: data, 'time': time_stamp}
            dd.update(d)
            fw_data = json.dumps(dd).encode('utf-8')

            # original
            # fw_data = json.dumps({target_json_path: data, 'time': time_stamp}).encode('utf-8')



        else:
            # fixme we need to make sure time is present in this data
            fw_data = data

        logging.debug("Forwarding '%r' as '%r' to %s(%s)" % (body, fw_data, target_exchange, target_routing_key))
        out_channel.basic_publish(target_exchange, target_routing_key, fw_data)

    if len(source_exchange) > 0:
        logging.debug("Declaring in-exchange {0} for signal {1}".format(source_exchange, sig))
        in_channel.exchange_declare(exchange=source_exchange, exchange_type='direct')

    logging.debug("Declaring in-queue for signal {0}".format(sig))
    result = in_channel.queue_declare(queue='', durable=False,
                                      auto_delete=True)  # we just want an auto name queue=in_server_exchange_name,

    logging.debug(
        "Binding in-queue for signal {0} to exchange {1} with routing key {2}".format(sig, source_exchange,
                                                                                      source_routing_key))
    in_channel.queue_bind(exchange=source_exchange, routing_key=source_routing_key,
                          queue=result.method.queue)  # , routing_key=queue_name)

    logging.debug(
        "Starting consumer thread for signal {0}".format(sig))
    in_channel.basic_consume(queue=result.method.queue, auto_ack=True, exclusive=True,
                             on_message_callback=callback)
    in_channel.start_consuming()


def process(conf):
    workers = []
    for signal in conf['signals'].keys():
        source_exchange = conf['signals'][signal]['source']['exchange']
        source_routing_key = conf['signals'][signal]['source']['routing_key']
        source_datatype = conf['signals'][signal]['source']['datatype']

        target_exchange = conf['signals'][signal]['target']['exchange']
        target_routing_key = conf['signals'][signal]['target']['routing_key'] if 'routing_key' in \
                                                                                 conf['signals'][signal][
                                                                                     'target'] else ''
        target_datatype = conf['signals'][signal]['target']['datatype']
        target_json_pack = 'JSON' in conf['signals'][signal]['target']['pack'] if 'pack' in conf['signals'][signal][
            'target'] else False
        target_json_path = conf['signals'][signal]['target']['path'] if 'path' in conf['signals'][signal][
            'target'] else None

        workers.append(threading.Thread(target=process_signal_thread, args=(conf,
                                                                            signal, source_exchange, source_routing_key,
                                                                            target_exchange, target_routing_key,
                                                                            target_json_pack, target_json_path,
                                                                            source_datatype, target_datatype),
                                        daemon=True))

    for t in workers:
        t.start()

    for t in workers:
        t.join()


def main():
    options = argparse.ArgumentParser()
    options.add_argument("-c", '--config', dest="conf", type=str, required=True, help='Path to a yml config')
    options.add_argument("-v", "--verbose", required=False, help='Verbose', dest='verbose', action="store_true")
    options.add_argument("-l", "--log", dest="logLevel", choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                         help="Set the logging level")

    args = options.parse_args()

    if args.logLevel:
        logging.basicConfig(level=getattr(logging, args.logLevel))

    with open(args.conf, 'r') as f:
        conf = yaml.load(f, Loader=yaml.FullLoader)
        process(conf)


if __name__ == '__main__':
    main()
