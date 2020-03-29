# MCB调试通讯格式

#### 日志相关

```json
{   
	"cmd": "set_log",
    "arg": {
        "log_level": 4
    }
}

```

> `cmd`  cmd name
>
> `log_level` see std_common.h



#### set mac

```json
{    
	"cmd": "set_mac"
}

```

> `cmd`  cmd name



#### MCB相关

#### send2root

##### *send message to root*

```json
{   
	"cmd": "send2root"
}

```

> `cmd`  cmd name



#### send2node

##### *send message to node*

```json
{   
	"cmd": "send2node",
    "arg": {
        "mac": "FF:FF:FF:FF:FF:FF"
    }
}

```

> `cmd`  cmd name
>
> `mac` target mac



#### broadcast

##### *broadcast to all node*

```json
{   
	"cmd": "broadcast"
}

```

> `cmd`  cmd name



#### group

##### *group send*

```json
{   
	"cmd": "group"
}

```

> `cmd`  cmd name



>