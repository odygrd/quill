import re
import json
import urllib.parse


def generate_chart_url(markdown_table, title):
    # Define a static color mapping with bright and distinct colors
    color_mapping = {
        'Quill': 'rgba(0,128,0,0.6)',  # Green
        'fmtlog': 'rgba(255,99,132,0.6)',  # Light Red
        'spdlog': 'rgba(0,128,128,0.6)',  # Teal
        'Reckless': 'rgba(255,99,71,0.6)',  # Tomato Red
        'XTR': 'rgba(255,206,86,0.6)',  # Yellow
        'BqLog': 'rgba(75,0,130,0.6)',  # Indigo
        'Quill - Macro Free Mode': 'rgba(0,206,209,0.6)',  # Dark Turquoise
        'Boost.Log': 'rgba(106,90,205,0.6)'  # Slate Blue
    }

    # Parse the markdown table
    lines = markdown_table.strip().split("\n")[2:]  # Skip header lines
    datasets = []

    # Process each row in the table
    for line in lines:
        parts = re.split(r'\s*\|\s*', line.strip('| '))
        library_name = re.sub(r'\[|\]\(.*?\)', '', parts[0])  # Remove markdown link syntax

        # Skip MS BinLog from being included in the chart
        if library_name == 'MS BinLog':
            continue

        data = float(parts[1])  # Extract the throughput value (in million msg/second)

        dataset = {
            'label': library_name,
            'data': [data],
            'backgroundColor': color_mapping.get(library_name, 'rgba(0,0,0,0.6)'),  # Default color
            'barPercentage': 0.9,  # Adjust bar width
            'categoryPercentage': 0.9  # Adjust spacing between bars
        }
        datasets.append(dataset)

    # Construct the QuickChart URL
    chart_config = {
        'type': 'bar',
        'data': {
            'labels': ['Throughput (million msg/second)'],  # Single bar for throughput
            'datasets': datasets
        },
        'options': {
            'title': {
                'display': True,
                'text': title
            },
            'scales': {
                'yAxes': [{
                    'type': 'logarithmic',
                    'scaleLabel': {
                        'display': True,
                        'labelString': 'Latency (ns)'
                    },
                    'ticks': {
                        'min': 1,
                        'maxTicksLimit': 10,
                    }
                }],
            },
            'plugins': {
                'datalabels': {
                    'display': False
                }
            }
        }
    }

    base_url = "https://quickchart.io/chart"
    chart_json = json.dumps(chart_config)
    encoded_chart = urllib.parse.quote(chart_json)
    chart_url = f"{base_url}?c={encoded_chart}"

    return chart_url


# Define the markdown table
markdown_table = """
| Library                                                           | million msg/second | elapsed time |
|-------------------------------------------------------------------|:------------------:|:------------:|
| [MS BinLog (binary log)](http://github.com/Morgan-Stanley/binlog) |       62.93        |    62 ms     |
| [BqLog (binary log)](https://github.com/Tencent/BqLog)            |       14.71        |    271 ms    |
| [XTR](https://github.com/choll/xtr)                               |        8.16        |    490 ms    |
| [Quill](http://github.com/odygrd/quill)                           |        5.24        |    763 ms    |
| [spdlog](http://github.com/gabime/spdlog)                         |        4.32        |    925 ms    |
| [fmtlog](http://github.com/MengRao/fmtlog)                        |        2.82        |   1417 ms    |
| [Reckless](http://github.com/mattiasflodin/reckless)              |        2.72        |   1471 ms    |
| [Quill - Macro Free Mode](https://github.com/choll/xtr)           |        2.65        |   1510 ms    |
| [BqLog](https://github.com/Tencent/BqLog)                         |        2.60        |   1537 ms    |
| [Boost.Log](https://www.boost.org)                                |        0.39        |   10102 ms   |
"""

# Generate the chart URL without MS BinLog
title = "Logging Library Throughput Comparison"
url = generate_chart_url(markdown_table, title)

# Print the generated chart URL
print(f"{url}")
