# HTAPを実現する分散DBのアーキテクチャ

## 概観

Single Master + Multiple Slave型のアーキテクチャをとる。
etcdが全体のコーディネーションを行う。
基本的にはMasterがOLTPリクエストを受け付け、Slaveが全体でOLAPリクエストを処理する。
処理の実行パスは3種類用意される。
1. MasterTxn: Masterで更新を含むトランザクションのログをSlaveに伝搬させ永続化させる。
2. ReadOnlyTxn: Slaveに含まれるサーバ群でデータを読みだしてRead-Onlyなトランザクションを実行する。
3. DistributedTxn: SlaveとMasterが協調し同一系列のエポックの中で巨大トランザクションを並列実行する、このトランザクションは同時に1つまでしか実行できない。
Masterのメモリの大きさが1によるトランザクション処理の限界となる。
それを超える大きさのトランザクション処理は3を通る。

### MasterTxn

基本的にはIn-Memory DBのような構造で、いわゆるARIES的な処理は行わない。
ページという1MBの単位でDB内部のデータを細切りにし、Masterのメモリに乗っているデータまでを1回のトランザクション処理の対象の上限とする。
メモリに載り切らないページもSlave側のクラスタに保存されているのでDBとしての容量とトランザクション処理サイズ上限に乖離が発生する。
ページ内のデータが全てCommitされたクリアな状態になったらそのメモリはいつでも破棄可能になる。
処理対象データがページに載っていない場合、バックエンドのSlave側からページデータをMasterのメモリに展開する。
Slaveに対してはRedoログしか転送せず、Commit済みのページは転送を行わない。Redoログのみからページそのものが生成可能だからである。
ログの形式はページIDと論理ログの組み合わせからなる。

### ReadOnlyTxn

Slave側は全Redoログを保持しており、Epochが進むたびにそこまでのRedoログを全部適用した状態で足並みを揃える。
Epochの途中のデータはReadOnlyなトランザクションからは獲得不能。
基本的にただのselectやaggregateやjoinを用いた探索的クエリを実行するための分散処理基盤である。
設計としてはPrestoなどに近いが、エグゼキュータはC++などの速度重視言語でプランナーはPythonなどの抽象度の高い言語で柔軟なクエリプランニングを実現する。


### DistributedTxn

TBD

## 基本構造

### Master

トランザクション処理を基本的にインメモリで高速に実行しRedoログを生成して転送する事に徹する。
更新系トランザクションは基本的に全てこのサーバに送られる。
クエリ処理はバッチ処理であることもあるので、SQLとDSLをサポートする。
SQLやDSLで記述された処理を中間言語にコンパイルし、ループなどを展開する事で仮想関数などに起因する従来のオーバーヘッドを迂回する。
中間言語はテーブルとそれに対する処理を記述できる簡潔な手続き処理言語であり、タプルをファーストクラスとして利用できる。
基本的にはSiloのEpochとログ並列化に、Aetherのflush-pipeliningを併用する。
ログはpre-commitが完了したものから順にSlaveのログ永続層に送られ、全部のack済みのログのEpochがインクリメントされたタイミングでユーザにcommit完了を通知する。

### Slave

Redoログをレプリケートする永続層と、そのログの中身のうち自分が作成すべきベージの分だけをリプレイしてページを保持するページストア層から成る。
ログは発生したそばから配信木を用いて低レイテンシで複数のSlaveに速やかに伝搬され全てのSlaveで永続化される。永続化が完了したらそれぞれがMasterへackを返却する。
ackが過半数を超えた段階でMasterはログの永続化を認める。
ページストア層はメタデータサーバが監理するページ分担に伴って自分が管理すべきページ群をローカルで維持し続ける。
永続化されたログが新しいepochに至るたびに既存のページに対してRCUのように新しいページを作成する。
既存のページは不変であり、snapshot isolationをread onlyなクエリに対して実現するために保持されるがSlaveの全体クロックが過ぎたら読まれる事がなくなるのでRCU的にページごと削除される。
これによって巨大なJOINを含む演算が時間的に数epochに跨ってもページが途中で消去されることはない。


## 詳細設計

### トランザクション中間言語


#### 変数

##### 値


```
<value>
```

##### 値例
```
42
"str"
["array", "supported", "by", "literal"]
{"tuple", 1, 2, 3}
```

##### 代入

```
let <variable name> = <value>
```

##### 代入例

```
let a = 10
let arr = [1 2 3 4]
let <int:a, int:b> tuple = {4, 5}
```

##### テーブルへの挿入

```
insert my_table, tuple
```

#### 計算

##### 算術演算

```
1 + 2 + 3
8 - 1 - 2
2 * 3 * 7
15 / 5 / 2
15.0 / 5 / 2 # 1.5(未サポート)
```

#### 制御文

##### トランザクション

```
transaction <isolation level> {
  ...
}
```

##### 条件分岐

```
if <condition> {
  <true sequence>
} [else {
  <false sequence>
}]
```


##### 画面出力（デバッグ用）

```
print_int(<variable name>)
```

##### データ出力

```
emit <row>
```

##### データ出力例

タプルを出力することでクエリ結果を外部へ出す

```
emit tuple
```

Emitted value's types are statically checked.

#### テーブルスキャン

テーブルのフルスキャンは構文を用意している

```
scan テーブル名, タプルの仮引数名 {
    <sequence of instruction>
}
```

##### テーブルスキャン例

例えば以下のようになる。

```
# SELECT b,c FROM foo;
define<{int:x key, int:y, int:z}> foo
transaction {
  scan foo, row {
	emit row
  }
}
```

```
# SELECT * FROM foo where b == 1;
define<{int:a key, int:b, int:c}> foo
transaction {
  scan foo, row {
    if row.b == 1 {
      emit row
    }
  }
}

```

```
# SELECT * FROM foo,bar where foo.a == bar.x;
# Nested loop join
define<{int:a key, int:b, int:c}> foo
define<{int:x key, int:y}> bar
transaction {
  scan foo, f_row {
    scan bar, b_row {
      if f_row.a == row.b {
        let <int:foo.a, int: foo.b, int:foo.c, int:bar.x, int:bar.y> joined = {f_row.a, f_row.b, f_row.c, b_row.x, b_row.y}
        emit joined
      }
    }
  }
}
```

