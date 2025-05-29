import re
import json
import urllib.parse


def generate_chart_url(markdown_table, title):
    # Define a static color mapping with bright and distinct colors
    color_mapping = {
        'Quill Bounded Dropping Queue': 'rgba(0,128,0,0.6)',  # Green
        'fmtlog': 'rgba(255,99,132,0.6)',  # Light Red
        'Quill Unbounded Queue': 'rgba(0,255,0,0.6)',  # Bright Green
        'PlatformLab NanoLog': 'rgba(255,159,64,0.6)',  # Orange
        'MS BinLog': 'rgba(153,102,255,0.6)',  # Purple
        'XTR': 'rgba(255,206,86,0.6)',  # Yellow
        'Reckless': 'rgba(255,99,71,0.6)',  # Tomato Red
        'Iyengar NanoLog': 'rgba(100,149,237,0.6)',  # Cornflower Blue
        'spdlog': 'rgba(0,128,128,0.6)',  # Teal
        'g3log': 'rgba(255,165,0,0.6)',  # Orange Red
        'BqLog': 'rgba(75,0,130,0.6)',  # Indigo
        'Quill Unbounded Queue - Macro Free Mode': 'rgba(0,206,209,0.6)'  # Dark Turquoise
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
| Library                                                                   | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill)            |  8   |  9   |  9   |  9   |  11  |   13   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)                   |  8   |  9   |  9   |  9   |  11  |   13   |
| [fmtlog](http://github.com/MengRao/fmtlog)                                |  8   |  9   |  10  |  10  |  12  |   13   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)              |  13  |  14  |  16  |  18  |  22  |   26   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                      |  20  |  21  |  21  |  22  |  60  |   95   |
| [XTR](https://github.com/choll/xtr)                                       |  7   |  7   |  29  |  31  |  33  |   54   |
| [Quill Unbounded Queue - Macro Free Mode](http://github.com/odygrd/quill) |  26  |  27  |  28  |  28  |  29  |   32   |
| [Reckless](http://github.com/mattiasflodin/reckless)                      |  26  |  28  |  31  |  32  |  34  |   42   |
| [BqLog](https://github.com/Tencent/BqLog)                                 |  55  |  57  |  61  |  87  | 152  |  167   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                   |  85  |  98  | 119  | 127  | 350  |  409   |
| [spdlog](http://github.com/gabime/spdlog)                                 | 148  | 151  | 154  | 157  | 165  |  173   |
| [g3log](http://github.com/KjellKod/g3log)                                 | 1191 | 1288 | 1367 | 1485 | 1600 |  1889  |
    """,
    """
| Library                                                                   | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [fmtlog](http://github.com/MengRao/fmtlog)                                |  9   |  9   |  9   |  10  |  12  |   13   |
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill)            |  8   |  9   |  10  |  10  |  12  |   15   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)                   |  8   |  9   |  10  |  10  |  13  |   15   |
| [XTR](https://github.com/choll/xtr)                                       |  7   |  8   |  9   |  10  |  31  |   39   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)              |  14  |  16  |  19  |  22  |  26  |   30   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                      |  21  |  21  |  22  |  23  |  61  |  102   |
| [Reckless](http://github.com/mattiasflodin/reckless)                      |  18  |  22  |  25  |  27  |  31  |   49   |
| [Quill Unbounded Queue - Macro Free Mode](http://github.com/odygrd/quill) |  28  |  29  |  30  |  31  |  34  |   41   |
| [BqLog](https://github.com/Tencent/BqLog)                                 |  57  |  60  |  64  |  95  | 162  |  179   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                   |  63  |  93  | 127  | 134  | 228  |  337   |
| [spdlog](http://github.com/gabime/spdlog)                                 | 215  | 251  | 320  | 357  | 449  |  734   |
| [g3log](http://github.com/KjellKod/g3log)                                 | 1276 | 1347 | 1409 | 1462 | 1677 |  1954  |
    """,
    """
| Library                                                                   | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill)            |  10  |  12  |  13  |  13  |  15  |   16   |
| [fmtlog](http://github.com/MengRao/fmtlog)                                |  10  |  12  |  13  |  13  |  15  |   16   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)                   |  12  |  13  |  14  |  15  |  17  |   19   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                      |  23  |  23  |  24  |  25  |  63  |   97   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)              |  14  |  16  |  18  |  20  |  30  |   34   |
| [XTR](https://github.com/choll/xtr)                                       |  8   |  8   |  28  |  29  |  33  |   53   |
| [Quill Unbounded Queue - Macro Free Mode](http://github.com/odygrd/quill) |  31  |  32  |  33  |  34  |  35  |   37   |
| [BqLog](https://github.com/Tencent/BqLog)                                 |  57  |  60  |  64  |  88  | 160  |  173   |
| [Reckless](http://github.com/mattiasflodin/reckless)                      |  90  | 107  | 113  | 116  | 122  |  131   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                   |  88  |  99  | 119  | 128  | 357  |  424   |
| [spdlog](http://github.com/gabime/spdlog)                                 | 126  | 129  | 132  | 135  | 142  |  151   |
| [g3log](http://github.com/KjellKod/g3log)                                 | 899  | 975  | 1037 | 1121 | 1267 |  1453  |
    """,
    """
| Library                                                                   | 50th | 75th | 90th | 95th | 99th | 99.9th |
|---------------------------------------------------------------------------|:----:|:----:|:----:|:----:|:----:|:------:|
| [fmtlog](http://github.com/MengRao/fmtlog)                                |  10  |  12  |  13  |  14  |  16  |   19   |
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill)            |  12  |  13  |  14  |  15  |  16  |   18   |
| [XTR](https://github.com/choll/xtr)                                       |  8   |  12  |  13  |  15  |  30  |   40   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)                   |  13  |  14  |  16  |  17  |  20  |   23   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)                      |  23  |  24  |  25  |  26  |  65  |  105   |
| [Quill Unbounded Queue - Macro Free Mode](http://github.com/odygrd/quill) |  30  |  31  |  33  |  34  |  36  |   41   |
| [PlatformLab NanoLog](http://github.com/PlatformLab/NanoLog)              |  16  |  19  |  28  |  36  |  44  |   51   |
| [BqLog](https://github.com/Tencent/BqLog)                                 |  60  |  64  |  68  |  95  | 170  |  186   |
| [Reckless](http://github.com/mattiasflodin/reckless)                      |  79  |  93  | 101  | 105  | 112  |  131   |
| [Iyengar NanoLog](http://github.com/Iyengar111/NanoLog)                   |  87  |  95  | 128  | 135  | 195  |  330   |
| [spdlog](http://github.com/gabime/spdlog)                                 | 197  | 224  | 276  | 306  | 394  |  689   |
| [g3log](http://github.com/KjellKod/g3log)                                 | 1000 | 1062 | 1131 | 1203 | 1374 |  1617  |
    """,
    """
| Library                                                        | 50th  | 75th  | 90th  | 95th  | 99th  | 99.9th |
|----------------------------------------------------------------|:-----:|:-----:|:-----:|:-----:|:-----:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill) |  48   |  52   |  56   |  59   |  123  |  158   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  67   |  69   |  72   |  73   |  79   |  280   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)        |  126  |  136  |  145  |  151  |  160  |  172   |
| [XTR](https://github.com/choll/xtr)                            |  287  |  295  |  342  |  347  |  355  |  576   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  649  |  668  |  702  |  723  |  753  |  790   |
| [spdlog](http://github.com/gabime/spdlog)                      | 11659 | 11758 | 11848 | 11905 | 12866 | 13543  |
    """,
    """
| Library                                                        | 50th  | 75th  | 90th  | 95th  |  99th  | 99.9th |
|----------------------------------------------------------------|:-----:|:-----:|:-----:|:-----:|:------:|:------:|
| [Quill Bounded Dropping Queue](http://github.com/odygrd/quill) |  51   |  54   |  57   |  59   |   62   |   78   |
| [MS BinLog](http://github.com/Morgan-Stanley/binlog)           |  69   |  72   |  74   |  76   |   82   |  299   |
| [Quill Unbounded Queue](http://github.com/odygrd/quill)        |  76   |  83   |  90   |  95   |  105   |  119   |
| [fmtlog](http://github.com/MengRao/fmtlog)                     |  675  |  700  |  742  |  759  |  790   |  822   |
| [XTR](https://github.com/choll/xtr)                            |  580  | 1210  | 1309  | 1371  | 198694 | 222254 |
| [spdlog](http://github.com/gabime/spdlog)                      | 12128 | 12247 | 12363 | 12460 | 13910  | 15902  |
    """
]

titles = ["Logging Numbers - 1 Thread Logging", "Logging Numbers - 4 Thread Logging",
          "Logging Large Strings - 1 Thread Logging", "Logging Large Strings - 4 Thread Logging",
          "Logging Vector - 1 Thread Logging", "Logging Vector - 4 Thread Logging"]

# Generate URLs for each chart
chart_urls = []
for markdown_table, title in zip(markdown_tables, titles):
    url = generate_chart_url(markdown_table, title)
    chart_urls.append(url)

# Print all generated URLs
for i, url in enumerate(chart_urls, start=1):
    print(f"Chart URL {i}: {url}")
