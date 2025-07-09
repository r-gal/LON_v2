#ifndef LON_INTERFACE_PHY_H
#define LON_INTERFACE_PHY_H


class LonInterfacePhy_c
{

  

  protected:

  const uint8_t portNo;

  bool getClk(void);
  bool getDat(void);

  void setClkPhy(bool data);
  void setDatPhy(bool data);

  public:


  LonInterfacePhy_c(uint8_t portNo_);

  static void InitInterfaceTick(void);




};

#endif
