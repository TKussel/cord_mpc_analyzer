#!/bin/env/python
import argparse
from collections import namedtuple
import datetime
import pandas as pd
import mpc_histogram

############## Age Pyramide

def get_extinguished_age_data(server, port, diagnosis):
    query = '/fhir/Patient?_has:Condition:subject:code='+diagnosis+'&__columns=id%3AgetIdPart(id)%40join(%22%20%22)%2Cgender%3Agender%40join(%22%2C%20%22)%2CbirthDate%3AbirthDate.substring(0%5C%2C%204)%40join(%22%2C%20%22)'
    data = pd.read_csv(server + ':' + port + query)
    return data

def calculate_age_from_birthyear(data):
    thisyear = datetime.date.today().year
    data['age'] = thisyear - data['birthDate']
    return data

def bin_agePyramide(data):
    bin_edges = [0,10,20,30,40,50,60,70,80,90,999]

    categories = [pd.Interval(-0.001, 10, closed='right'),
                  pd.Interval(10, 20, closed='right'),
                  pd.Interval(20, 30, closed='right'),
                  pd.Interval(30, 40, closed='right'),
                  pd.Interval(40, 50, closed='right'),
                  pd.Interval(50, 60, closed='right'),
                  pd.Interval(60, 70, closed='right'),
                  pd.Interval(70, 80, closed='right'),
                  pd.Interval(80, 90, closed='right'),
                  pd.Interval(90, 999, closed='right')]
    data['age'] = pd.cut(data['age'], bins=bin_edges, include_lowest=True)
    data = data.value_counts(subset=['gender', 'age'])
    indices = data.index.to_flat_index().array
    male = [0] * len(categories)
    female = [0] * len(categories)
    for (i, j), row in zip(indices, data):
        # print(i)
        position = categories.index(j)
        if i == 'male':
            male[position] = row
        else:
            female[position] = row
    return (categories, male, female)

def run_agePyramideHistogram(male, female, connection_settings, k):
    my_id, parties = connection_settings
    parties = list(convert_party(parties))
    print('Calculating Male Histogram')
    result_male = mpc_histogram.create_histogram(my_id, parties, male, k)
    print('Calculating Female Histogram')
    result_female = mpc_histogram.create_histogram(my_id, parties, female, k)
    return (result_male, result_female)

def combine_agePyramide_results(categories, male, female):
    result = pd.DataFrame()
    result['age'] = categories
    result['male'] = male
    result['female'] = female
    return result

def print_agePyramide_result(result, connection_settings, k):
    print('Result of Age Pyramide Computation between {} Parties:'.format(len(connection_settings[1])))
    print(result)
    print('Note, that results with less or equal {} counts have been supressed'.format(k))

############## Time Series
def get_extinguished_time_data(server, port, variant):
    query = ''
    if variant == 'covid-19':
        query = '/fhir/Condition?code=J12.8,U07.1,U08.9,U09.9&__columns=condition_id%3AgetIdPart(id)%40join(%22%2C%20%22)%2Ccode%3Acode.coding.where(system%3D\'http%3A%2F%2Ffhir.de%2FCodeSystem%2Fdimdi%2Ficd-10-gm\').code%40join(%22%2C%20%22)%2Cdisplay%3Acode.coding.where(system%3D\'http%3A%2F%2Ffhir.de%2FCodeSystem%2Fdimdi%2Ficd-10-gm\').display%40join(%22%2C%20%22)%2Cpatient_id%3AgetIdPart(subject.reference)%40join(%22%2C%20%22)%2Cencounter_id%3AgetIdPart(encounter.reference)%40join(%22%2C%20%22)%2CrecordedDate%3ArecordedDate%40join(%22%2C%20%22)'
    elif variant == 'cf-pku':
        query = '/fhir/Condition?code=E70.0,E70.1,E84.0,E84.1,E84.8,E84.80,E84.87,E84.88,E84.9&__columns=condition_id%3AgetIdPart(id)%40join(%22%2C%20%22)%2Ccode%3Acode.coding.where(system%3D\'http%3A%2F%2Ffhir.de%2FCodeSystem%2Fdimdi%2Ficd-10-gm\').code%40join(%22%2C%20%22)%2CrecordedDate%3ACondition.recordedDate%40join(%22%20%22)%2Cpatient_id%3AgetIdPart(subject.reference)%40join(%22%2C%20%22)%2Cencounter_id%3AgetIdPart(encounter.reference)%40join(%22%2C%20%22)%2Cdisplay%3Acode.coding.where(system%3D\'http%3A%2F%2Ffhir.de%2FCodeSystem%2Fdimdi%2Ficd-10-gm\').display%40join(%22%2C%20%22)%2Cname_use%3Asubject.resolve().name.use%40join(%22%2C%20%22)%2Cname_family%3Asubject.resolve().name.family%40join(%22%2C%20%22)%2Cname_given%3Asubject.resolve().name.given%40join(%22%2C%20%22)%2Cgender%3Asubject.resolve().gender%40join(%22%2C%20%22)%2Cbirthdate%3Asubject.resolve().birthdate%40join(%22%2C%20%22)'
    data = pd.read_csv(server + ':' + port + query)
    data['recordedDate'] = data['recordedDate'].transform(lambda x: pd.to_datetime(x))
    return data

def bin_timeSeries(data, time_range):
    start, end = time_range
    duration = pd.to_datetime(end).to_period('M') - pd.to_datetime(start).to_period('M')
    data_bin = pd.date_range(pd.to_datetime(start), periods=duration.n+2, freq='MS')
    data['recordedMonth'] = pd.cut(data['recordedDate'], bins=data_bin, include_lowest=True, right=False)
    grouped = data.groupby('recordedMonth').size()
    data_frame = pd.DataFrame({'month': grouped.index.to_list(), 'count': grouped})
    data_frame.reset_index(drop=True, inplace=True)
    return data_frame

def run_timeSeriesHistogram(data, connection_settings, k):
    my_id, parties = connection_settings
    parties = list(convert_party(parties))
    print('Calculating Time Series Histogram')
    data['count'] = mpc_histogram.create_histogram(my_id, parties, data['count'].to_list(), k)
    return data

def print_timeSeries(result, connection_settings, variant, k):
    print('Result of TimeSeries Computation (variant {}) between {} Parties:'.format(variant, len(connection_settings[1])))
    print(result)
    print('Note, that results with less or equal {} counts have been supressed'.format(k))


############## Diagnose Coincidence
def get_extinguished_diag_data(server, port):
    query = '/fhir/Patient?_has:Condition:subject:code=E84.0,E84.1,E84.8,E84.80,E84.87,E84.88,E84.9&_has:Condition:subject:code=O30,O30.0,O30.1,O30.2,O30.8,O30.9&_revinclude=Condition:subject&__columns=id%3AgetIdPart(Condition.id)%40join(%22%2C%20%22)%2Csubject%3AgetIdPart(Condition.subject.reference)%40join(%22%2C%20%22)%2Cicd10%3Acode.coding.where(system%3D\'http%3A%2F%2Ffhir.de%2FCodeSystem%2Fdimdi%2Ficd-10-gm\').extension(\'http%3A%2F%2Ffhir.de%2FStructureDefinition%2Ficd-10-gm-primaercode\').value.code%40join(%22%2C%20%22)%2Cgender%3ACondition.subject.resolve().gender%40join(%22%2C%20%22)'
    data = pd.read_csv(server + ':' + port + query).dropna()
    return data



############## Algorithm Dispatch

def call_algo(fhir_server, fhir_port, algo, connection_settings, k):
    if algo[0] == 'agePyramide':
        print("Requesting FHIR Data")
        data = get_extinguished_age_data(fhir_server, fhir_port, algo[1])
        print("Data Recieved")
        categories, male, female = bin_agePyramide(calculate_age_from_birthyear(data))
        result_male, result_female = run_agePyramideHistogram(male, female, connection_settings, k)
        results = combine_agePyramide_results(categories, result_male, result_female)
        print_agePyramide_result(results, connection_settings, k)
    if algo[0] == 'timeSeries':
        print("Requesting FHIR Data")
        data = get_extinguished_time_data(fhir_server, fhir_port, algo[1])
        print("Data Recieved")
        data = bin_timeSeries(data, algo[2])
        result = run_timeSeriesHistogram(data, connection_settings, k)
        print_timeSeries(result, connection_settings, algo[1], k)
    if algo[0] == 'diagCoincidence':
        print("Requesting FHIR Data")
        data = get_extinguished_diag_data(fhir_server, fhir_port)
        print("Data Recieved")
        print('Not implemented yet')
        print(data)


Party = namedtuple('Party', ['id', 'ip', 'port'])
def parse_connection_settings(args):
    my_id = args.id
    parties_strings = args.parties.split(';')
    parties = []
    for p in parties_strings:
        rest, port = p.split(':')
        p_id, ip = rest.split('@')
        parties.append(Party(int(p_id), ip, int(port)))
    return (my_id, parties)

def convert_party(parties):
    return map(lambda x: (x.id, x.ip, x.port), parties)



def main():
    ver = '0.0.1'
    parser = argparse.ArgumentParser(prog='CORD_MI MPC version ' + ver, description='CORD_MI Cookbook analyses using MPC')
    parser.add_argument('-fs', '--fhirserver', required=True, help="URL of the FHIR Extingusher server")
    parser.add_argument('-fp', '--fhirport', required=True, help="Port of the FHIR Extingusher server")
    parser.add_argument('-a', '--algorithm', choices=['agePyramide', 'diagCoincidence', 'timeSeries'],required=True, help="Which analysis to perform")
    parser.add_argument('-d', '--diagnosis', default="I11.00", help="Diagnosis to query for age pyramide analysis")
    parser.add_argument('-v', '--variant', choices=['covid-19', 'cf-pku'], help="Variant of time series analysis")
    parser.add_argument('-i', '--id', required=True, type=int, help="ID of the local party")
    parser.add_argument('-p', '--parties', required=True, help="String giving all parties connection information in the form \"ID@IP:Port;NextID@NextIP:NextPort;...\"")
    parser.add_argument('-k', '--kthreshold', default=5, type=int, help="Threshold value for k anonymous results. k=0 desables the anonymization.")
    # parser.add_argument('-g', '--graphics', default=False, type=bool, help="Plot results")
    parser.add_argument('-ts', '--timestart', help="Start month for time series analysis in the format YYYY-MM")
    parser.add_argument('-te', '--timeend', help="End month for time series analysis in the format YYYY-MM")
    args = parser.parse_args()
    fhir_server = args.fhirserver
    fhir_port = args.fhirport
    algo = args.algorithm
    k = args.kthreshold

    if algo == 'agePyramide':
        algo = (algo, args.diagnosis)
    elif algo == 'timeSeries':
        algo = (algo, args.variant, (args.timestart, args.timeend))
    elif algo == 'diagCoincidence':
        algo = [algo]

    # if args.graphics:
        # print("Plot output not implemented, yet")

    connection_settings = parse_connection_settings(args)
    call_algo(fhir_server, fhir_port, algo, connection_settings, k)


if __name__ == "__main__":
    main()
