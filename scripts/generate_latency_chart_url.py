import re
import json
import urllib.parse

def generate_chart_url(markdown_table, title):
    # Define a static color mapping with bright and distinct colors
    color_mapping = {
        'Quill Bounded Dropping Queue': 'rgba(0,128,0,0.6)',    # Green
        'fmtlog': 'rgba(255,99,132,0.6)',                        # Light Red
        'Quill Unbounded Queue': 'rgba(0,255,0,0.6)',           # Bright Green
        'PlatformLab NanoLog': 'rgba(255,159,64,0.6)',          # Orange
        'MS BinLog': 'rgba(153,102,255,0.6)',                    # Purple
        'XTR': 'rgba(255,206,86,0.6)',                           # Yellow
        'Reckless': 'rgba(255,99,71,0.6)',                       # Tomato Red
        'Iyengar NanoLog': 'rgba(100,149,237,0.6)',               # Cornflower Blue
        'spdlog': 'rgba(0,128,128,0.6)',                         # Teal
        'g3log': 'rgba(255,165,0,0.6)'                           # Orange Red
    }

    # Parse the markdown table
    lines = markdown_table.strip().split("\n")[2:]  # Skip header lines
    labels = ['50th', '75th', '90th', '95th', '99th', '99.9th']
    datasets = []

    # Process each row in the table
    for line in lines:
        parts = re.split(r'\s*\|\s*', line.strip('| '))
        library_name = re.sub(r'\[|\]\(.*?\)', '', parts[0])  # Remove markdown link syntax
        data = [int(x) for x in parts[1:]]

        # Cap data values at 100 ns
        capped_data = [min(x, 100) for x in data]

        dataset = {
            'label': library_name,
            'data': capped_data,
            'backgroundColor': color_mapping.get(library_name, 'rgba(0,0,0,0.6)'),  # Default color
            'barPercentage': 0.9,  # Adjust bar width
            'categoryPercentage': 0.9  # Adjust spacing between bars
        }
        datasets.append(dataset)

    # Construct the QuickChart URL
    chart_config = {
        'type': 'bar',
        'data': {
            'labels': labels,
            'datasets': datasets
        },
        'options': {
            'title': {
                'display': True,
                'text': title
            },
            'scales': {
                'yAxes': [{
                    'scaleLabel': {
                        'display': True,
                        'labelString': 'Latency (ns)'
                    },
                    'ticks': {
                        'max': 100
                    }
                }],
                'xAxes': [{
                    'scaleLabel': {
                        'display': True,
                        'labelString': 'Percentiles'
                    }
                }]
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

# Define multiple markdown tables and titles
markdown_tables = [
    """
| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |
|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill) |  48  |  50  |  53  |  55  |  58  |   62   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)        |  54  |  56  |  57  |  58  |  61  |   66   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  68  |  69  |  72  |  74  |  79  |  281   |
| [XTR](https://github.com/choll/xtr)                            | 284  | 294  | 340  | 346  | 356  |  575   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     | 711  | 730  | 754  | 770  | 804  |  834   |
| [spdlog](http://github.com/gabime/spdlog)                      | 6191 | 6261 | 6330 | 6386 | 6633 |  7320  |
    """,
    """
| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |
|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill) |  50  |  52  |  54  |  56  |  60  |   82   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  70  |  72  |  75  |  79  |  88  |  286   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)        |  97  | 107  | 116  | 122  | 135  |  148   |
| [XTR](https://github.com/choll/xtr)                            | 512  | 711  | 761  | 791  | 865  |  945   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     | 780  | 804  | 823  | 835  | 860  |  896   |
| [spdlog](http://github.com/gabime/spdlog)                      | 6469 | 6549 | 6641 | 6735 | 7631 |  9430  |
    """
]

titles = ["Logging Vector - 1 Thread Logging", "Logging Large Strings - 4 Thread Logging"]

# Generate URLs for each chart
chart_urls = []
for markdown_table, title in zip(markdown_tables, titles):
    url = generate_chart_url(markdown_table, title)
    chart_urls.append(url)

# Print all generated URLs
for i, url in enumerate(chart_urls, start=1):
    print(f"Chart URL {i}: {url}")
