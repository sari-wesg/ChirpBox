<!--
 * @Author: your name
 * @Date: 2020-04-24 13:53:57
 * @LastEditTime: 2020-04-25 19:56:10
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \mixer_dutycycle\Doc\chirp_sniff.md
 -->

```mermaid
graph TD;
init(Init sniff radio) -->id2[Set timer = 100 symbol time]
id2-->id1[Cad On]
id1-->id25[sniff.radio = SNIFF_CAD]
id25-->id26[sniff.state = CAD_DETECT]
id26-->id12[Timer IRQ]
id26-->id6[Radio IRQ]-->id13[Dio0]
id5[Rearrange SF]-->id15[Init sniff radio]

subgraph Timer_IRQ;
  c1{Cad detect Timeout? sniff.state == CAD_DETECT}--Yes-->id5
  c1--No-->c2{Valid header timeout? sniff.state == VALID_HEADER}
  c2--Yes-->id5
  c2--No-->c3{Node ID packet? sniff.state == NODE_ID_GET}
  c3--Yes-->id9[Add to list]
  id9-->id5
  end

subgraph Radio_IRQ;
  id7[Dio0]-->id16{sniff.radio == SNIFF_RX}
  id16--Yes-->id17[Rx Done]
  id16--No-->id18{sniff.radio == SNIFF_TX}
  id18--Yes-->id19[Tx Done]
  id18--No-->id20{sniff.radio == SNIFF_CAD}
  id20--Yes-->id10{Cad detected?}
  id10--Yes-->id21[Rx mode]
  id21-->id22[Set timer = Valid Header time]
  id22-->id23[sniff.radio = SNIFF_RX]
  id23-->id24[sniff.state = VALID_HEADER]-->id27[Radio_IRQ]-->id28[Dio3]
  id24-->id29[Timer IRQ]
  id10--No-->id14[Cad On]
  end
```

1. 方向
TB	从上到下
BT	从下到上
RL	从右到左
LR	从左到右

2. 连线类型
>	添加尾部箭头
-	不添加尾部箭头
–	单线
–text–	单线上加文字
==	粗线
==text==	粗线加文字
-.-	虚线
-.text.-	虚线加文字

1. 节点
表述	说明
id[文字]	矩形节点
id(文字)	圆角矩形节点
id((文字))	圆形节点
id>文字]	右向旗帜状节点
id{文字}	菱形节点