import pandas as pd
from plotly.subplots import make_subplots
import plotly.graph_objs as go
import glob
 
f = 'ur_robot.csv'
df = pd.read_csv(f, delim_whitespace=True)
 
# make output csv without time gaps
# df['time'] = df.at[0, 'time'] + df['seqno'] * 0.002
# df.to_csv(r'server/ur_robot_clean.csv', sep=' ', index = False)
 
df = df[['time', 'seqno']]
 
df['time_diff'] = df['time'].diff() * 1000
df.at[0, 'time_diff'] = 0
 
df['time_offset'] = (df['time'] - df.at[0, 'time']) * 1000
 
fig = go.Figure(data=go.Scatter(x=df['time_offset'], y=df['time_diff'], mode='markers'), layout_xaxis_range=[0,20000], layout_yaxis_range=[0,250])
 
fig.update_xaxes(title_text="data time (msec)")
fig.update_yaxes(title_text="data time gap (msec)")
 
fig.update_xaxes(tick0=0.0, dtick=1000)
 
fig.update_layout(margin_l=5, margin_t=5, margin_b=5, margin_r=5)
 
fig.show()
 
df.to_csv('ur_robot_time_gaps.csv' , sep=' ', index = False)