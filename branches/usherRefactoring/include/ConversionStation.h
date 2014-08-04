/**
 * @file ConversionStation.h
 *
 * @date Jul 21, 2014
 * @author Stefano Martina
 */

#ifndef CONVERSIONSTATION_H_
#define CONVERSIONSTATION_H_

#include "Property.h"

namespace material {

  class ConversionStation :public PropertyObject { //MaterialObject {
  public:
    enum Type {FLANGE, ENDCAP};

    ConversionStation(Type stationType) :
      stationType_ (stationType),
      type_ ("type", parsedAndChecked()),
      stationsNode_ ("Station", parsedOnly()),
      conversionsNode_ ("Conversion", parsedOnly()) {} ;
    virtual ~ConversionStation() {};

    virtual void build();
    virtual void routeServicesTo(MaterialObject& outputObject) const;

  private:
    static const std::map<Type, const std::string> typeString;
    Type stationType_;
    ReadonlyProperty<std::string, NoDefault> type_;
    PropertyNodeUnique<std::string> stationsNode_;
    PropertyNodeUnique<std::string> conversionsNode_;

    const std::string getTypeString() const;
    void buildConversions();

    class Conversion : public PropertyObject {
    public:
      PropertyNode<std::string> inputNode_;
      PropertyNode<std::string> outputNode_;

      Conversion() :
        inputNode_ ("Input", parsedAndChecked()),
        outputNode_ ("Output", parsedAndChecked()) {};
      virtual ~Conversion() {};

      void build();

      class Inoutput : public PropertyObject {
      public:
        PropertyNodeUnique<std::string> elementsNode_;

        Inoutput() :
          elementsNode_ ("Element", parsedOnly()) {};
        virtual ~Inoutput() {};

        void build();

        class Element : public PropertyObject {
        public:
          ReadonlyProperty<std::string, NoDefault> elementName;
          ReadonlyProperty<long, NoDefault> quantity;
          ReadonlyProperty<std::string, NoDefault> unit;
          ReadonlyProperty<bool, Default> exiting;

          Element() :
            elementName ("elementName", parsedAndChecked()),
            quantity ("quantity", parsedAndChecked()),
            unit ("unit", parsedAndChecked()),
            exiting ("exiting", parsedOnly(), false) {};
          virtual ~Element() {};

          //void build();
        };

        PtrVector<Element> elements;
      };

      PtrVector<Inoutput> inputs;
      PtrVector<Inoutput> outputs;
    };

    PtrVector<Conversion> conversions;
  };

} /* namespace material */

#endif /* CONVERSIONSTATION_H_ */
