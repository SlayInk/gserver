#include "db_async_store.h"
#include "mblock_pool.h"
#include "tb_msg_map.h"
#include "proxy_obj.h"
#include "ltime_act.h"
#include "sys_log.h"
#include "istream.h"
#include "message.h"

static ilog_obj *s_log = sys_log::instance()->get_ilog("base");
static ilog_obj *e_log = err_log::instance()->get_ilog("base");

namespace
{
  class sql : public sql_opt
  {
  public:
    virtual int id() { return 0; }
    ltime_act data_;
  };

  class tb_msg_update : public tb_msg_handler
  {
    class sql_update : public sql
    {
      virtual int opt_type() { return sql_opt::SQL_UPDATE; }
      virtual int build_sql(char *bf, const int bf_len)
      { return this->data_.update_sql(bf, bf_len); }
    };
  public:
    tb_msg_update(const int res) : resp_id_(res) { }
    virtual int handle_msg(proxy_obj *po, const char *msg, const int len)
    {
      int db_sid = -1;
      char bf[sizeof(ltime_act) + 4];
      stream_istr si(bf, sizeof(bf));

      in_stream is(msg, len);
      is >> db_sid >> si;
      assert(si.str_len() == sizeof(ltime_act));

      sql_update *sql = new sql_update();
      ::memcpy((char *)&sql->data_, si.str(), sizeof(ltime_act));
      db_async_store::instance()->do_sql(po->proxy_id(),
                                         db_sid,
                                         0,
                                         this->resp_id_,
                                         sql);
      return 0;
    }
    int resp_id_;
  };
  static tb_msg_maper tm_update(REQ_UPDATE_LIMIT_TIME_ACT, new tb_msg_update(RES_UPDATE_LIMIT_TIME_ACT));

}
