/**
 * @file Chauffeur.h
 *
 * @date 27/mar/2014
 * @author stefano
 */

#ifndef CHAUFFEUR_H_
#define CHAUFFEUR_H_

namespace materialRouting {

  class Chauffeur {
  public:
    Chauffeur();
    virtual ~Chauffeur();

    void arrange (Tracker& tracker, InactiveSurfaces& is, const std::list<Support*>& supports, bool printstatus = false);
    void arrangePixel (Tracker& tracker, InactiveSurfaces& is, bool printstatus = false);

  };

} /* namespace materialRouting */

#endif /* CHAUFFEUR_H_ */
