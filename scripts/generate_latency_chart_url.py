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
        'g3log': 'rgba(255,165,0,0.6)',  # Orange Red
        'BqLog': 'rgba(75,0,130,0.6)'  # Indigo
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
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill) |  11  |  13  |  13  |  14  |  15  |   16   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  11  |  12  |  13  |  14  |  15  |   17   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)        |  14  |  15  |  16  |  17  |  18  |   19   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  22  |  23  |  24  |  25  |  61  |  100   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)   |  15  |  17  |  21  |  27  |  33  |   39   |
| [XTR](https://github.com/choll/xtr)                            |  8   |  9   |  29  |  31  |  35  |   54   |
| [BqLog](https://github.com/Tencent/BqLog)                      |  29  |  30  |  31  |  51  |  60  |   71   |
| [Reckless](http://github.com/mattiasflodin/reckless)           |  91  | 107  | 115  | 118  | 124  |  135   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)        |  86  |  97  | 119  | 128  | 159  |  268   |
| [spdlog](http://github.com/gabime/spdlog)                      | 120  | 124  | 128  | 132  | 141  |  151   |
| [g3log](http://github.com/KjellKod/g3log)                      | 881  | 956  | 1018 | 1089 | 1264 |  1494  |
    """,
    """
| Library                                                        | 50th | 75th | 90th | 95th | 99th | 99.9th |
|----------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [XTR](https://github.com/choll/xtr)                            |  9   |  11  |  13  |  14  |  32  |   40   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  11  |  12  |  13  |  14  |  16  |   19   |
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill) |  13  |  14  |  15  |  16  |  17  |   19   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)        |  15  |  16  |  17  |  18  |  19  |   21   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  23  |  25  |  27  |  28  |  65  |  105   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)   |  16  |  20  |  32  |  38  |  44  |   51   |
| [BqLog](https://github.com/Tencent/BqLog)                      |  32  |  33  |  35  |  56  |  64  |   76   |
| [Reckless](http://github.com/mattiasflodin/reckless)           |  79  |  94  | 104  | 107  | 114  |  132   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)        |  85  |  93  | 125  | 133  | 168  |  237   |
| [spdlog](http://github.com/gabime/spdlog)                      | 178  | 218  | 261  | 281  | 381  |  651   |
| [g3log](http://github.com/KjellKod/g3log)                      | 992  | 1055 | 1121 | 1178 | 1360 |  1600  |
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
